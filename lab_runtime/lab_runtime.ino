#include <HTTPClient.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_mac.h>

#include "protocol_helpers.hpp"
#include "public_vectors.hpp"
#include "runtime_config.hpp"
#include "student_crypto.hpp"

namespace {
Preferences preferences;
String gAndrewId;
String gLastStationSsid;
String gLastResult;

struct SessionInfo {
    String sessionId;
    String physicalStationId;
    String logicalStationId;
    CryptoAlgorithm algorithm = CryptoAlgorithm::kUnknown;
    int shift = 0;
    int seed = 0;
    uint32_t e = 0;
    uint32_t d = 0;
    uint32_t n = 0;
    String nonce;
};

bool startsWith(const String& value, const char* prefix) {
    return value.startsWith(prefix);
}

String normalizeAndrewId(const String& raw) {
    String normalized = raw;
    normalized.trim();
    normalized.toLowerCase();
    return normalized;
}

String boardMacAddress() {
    uint8_t macBytes[6] = {0};
    if (esp_read_mac(macBytes, ESP_MAC_WIFI_STA) != ESP_OK) {
        String fallback = WiFi.STA.macAddress();
        fallback.toLowerCase();
        return fallback;
    }
    char buffer[18];
    snprintf(
        buffer,
        sizeof(buffer),
        "%02x:%02x:%02x:%02x:%02x:%02x",
        macBytes[0],
        macBytes[1],
        macBytes[2],
        macBytes[3],
        macBytes[4],
        macBytes[5]
    );
    String mac(buffer);
    mac.toLowerCase();
    return mac;
}

void saveAndrewId(const String& andrewId) {
    const String normalized = normalizeAndrewId(andrewId);
    preferences.putString("andrew_id", normalized);
    gAndrewId = normalized;
}

void loadAndrewId() {
    gAndrewId = normalizeAndrewId(preferences.getString("andrew_id", ""));
}

void printHelp() {
    Serial.println();
    Serial.println("Commands:");
    Serial.println("  ID <andrewid>   Set your Andrew ID");
    Serial.println("  RUN             Scan for a station and attempt the hunt step");
    Serial.println("  STATUS          Show saved settings and last result");
    Serial.println("  SELFTEST        Run public crypto sanity checks");
    Serial.println("  HELP            Show this message");
    Serial.println();
}

void printStatus() {
    Serial.println();
    Serial.print("Andrew ID: ");
    Serial.println(gAndrewId.length() > 0 ? gAndrewId : "<not set>");
    Serial.print("Board MAC: ");
    Serial.println(boardMacAddress());
    Serial.print("Last station SSID: ");
    Serial.println(gLastStationSsid.length() > 0 ? gLastStationSsid : "<none>");
    Serial.print("Last result: ");
    Serial.println(gLastResult.length() > 0 ? gLastResult : "<none>");
    Serial.println();
}

String buildJsonField(const String& key, const String& value, bool withComma) {
    std::string encoded = escapeJson(std::string(value.c_str()));
    String field = "\"" + key + "\":\"" + String(encoded.c_str()) + "\"";
    if (withComma) {
        field += ",";
    }
    return field;
}

String buildJsonField(const String& key, int value, bool withComma) {
    String field = "\"" + key + "\":" + String(value);
    if (withComma) {
        field += ",";
    }
    return field;
}

String postJson(const String& url, const String& body) {
    HTTPClient client;
    client.setTimeout(kHttpTimeoutMs);
    client.begin(url);
    client.addHeader("Content-Type", "application/json");
    int responseCode = client.POST(body);
    if (responseCode <= 0) {
        String error = client.errorToString(responseCode);
        client.end();
        return String("ERROR:") + error;
    }
    String response = client.getString();
    client.end();
    return response;
}

bool isErrorResponse(const String& response) {
    return response.startsWith("ERROR:") || response.indexOf("\"detail\"") >= 0;
}

String chooseBestStationSsid() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, true);
    delay(200);

    Serial.println("Scanning for nearby stations...");
    int networkCount = WiFi.scanNetworks();
    int bestIndex = -1;
    int bestRssi = -1000;
    for (int i = 0; i < networkCount; ++i) {
        String ssid = WiFi.SSID(i);
        if (!startsWith(ssid, kStationSsidPrefix)) {
            continue;
        }
        int rssi = WiFi.RSSI(i);
        if (rssi > bestRssi) {
            bestRssi = rssi;
            bestIndex = i;
        }
    }

    if (bestIndex < 0) {
        return String();
    }

    String ssid = WiFi.SSID(bestIndex);
    Serial.print("Selected station SSID: ");
    Serial.print(ssid);
    Serial.print(" (RSSI ");
    Serial.print(WiFi.RSSI(bestIndex));
    Serial.println(")");
    return ssid;
}

bool connectToStation(const String& ssid) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), kStationApPassword);
    Serial.print("Connecting to station AP");
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < kStationConnectTimeoutMs) {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to station AP.");
        return false;
    }
    Serial.print("Connected. Local IP: ");
    Serial.println(WiFi.localIP());
    gLastStationSsid = ssid;
    return true;
}

SessionInfo parseSessionInfo(const String& response) {
    SessionInfo info;
    std::string json(response.c_str());
    info.sessionId = extractJsonString(json, "sessionId").c_str();
    info.physicalStationId = extractJsonString(json, "physicalStationId").c_str();
    info.logicalStationId = extractJsonString(json, "logicalStationId").c_str();
    info.nonce = extractJsonString(json, "nonce").c_str();
    info.algorithm = algorithmFromString(extractJsonString(json, "algorithm"));
    info.shift = extractJsonInt(json, "shift", 0);
    info.seed = extractJsonInt(json, "seed", 0);
    info.e = static_cast<uint32_t>(extractJsonInt(json, "e", 0));
    info.d = static_cast<uint32_t>(extractJsonInt(json, "d", 0));
    info.n = static_cast<uint32_t>(extractJsonInt(json, "n", 0));
    return info;
}

String canonicalMessage(const SessionInfo& info) {
    return gAndrewId + "|" + info.nonce + "|" + info.logicalStationId;
}

String computeEncryptedResponse(const SessionInfo& info) {
    String message = canonicalMessage(info);
    switch (info.algorithm) {
        case CryptoAlgorithm::kCaesar: {
            return encryptCaesar(message, info.shift);
        }
        case CryptoAlgorithm::kLfsr: {
            std::vector<uint8_t> bytes = encryptLFSR(message, static_cast<uint8_t>(info.seed));
            return String(hexEncode(bytes).c_str());
        }
        case CryptoAlgorithm::kRsa: {
            std::vector<uint32_t> blocks = encryptRSA(message, info.e, info.n);
            return String(csvEncode(blocks).c_str());
        }
        default:
            return String();
    }
}

String decryptClue(const SessionInfo& info, const String& clueCiphertext) {
    std::string encoded(clueCiphertext.c_str());
    switch (info.algorithm) {
        case CryptoAlgorithm::kCaesar:
            return decryptCaesar(clueCiphertext, info.shift);
        case CryptoAlgorithm::kLfsr: {
            std::vector<uint8_t> bytes = hexDecode(encoded);
            return decryptLFSR(bytes, static_cast<uint8_t>(info.seed));
        }
        case CryptoAlgorithm::kRsa: {
            std::vector<uint32_t> blocks = csvDecodeU32(encoded);
            return decryptRSA(blocks, info.d, info.n);
        }
        default:
            return "Unable to decode clue.";
    }
}

bool runPublicSelfTest() {
    bool ok = true;

    if (stepLFSR(0xA7) != 0x53) {
        Serial.println("FAIL: stepLFSR(0xA7) should be 0x53");
        ok = false;
    }

    for (const CaesarVector& vec : kCaesarVectors) {
        String encrypted = encryptCaesar(vec.input, vec.shift);
        String decrypted = decryptCaesar(vec.encrypted, vec.shift);
        if (encrypted != vec.encrypted) {
            Serial.print("FAIL: Caesar encrypt mismatch for input '");
            Serial.print(vec.input.c_str());
            Serial.println("'");
            ok = false;
        }
        if (decrypted != vec.input) {
            Serial.print("FAIL: Caesar decrypt mismatch for vector '");
            Serial.print(vec.encrypted.c_str());
            Serial.println("'");
            ok = false;
        }
    }

    for (const LfsrVector& vec : kLfsrVectors) {
        std::vector<uint8_t> encrypted = encryptLFSR(vec.input, vec.seed);
        String decrypted = decryptLFSR(vec.encrypted, vec.seed);
        if (encrypted != vec.encrypted) {
            Serial.print("FAIL: LFSR encrypt mismatch for input '");
            Serial.print(vec.input.c_str());
            Serial.println("'");
            ok = false;
        }
        if (decrypted != vec.input) {
            Serial.print("FAIL: LFSR decrypt mismatch for input '");
            Serial.print(vec.input.c_str());
            Serial.println("'");
            ok = false;
        }
    }

    for (const RsaVector& vec : kRsaVectors) {
        std::vector<uint32_t> encrypted = encryptRSA(vec.input, vec.e, vec.n);
        String decrypted = decryptRSA(vec.encrypted, vec.d, vec.n);
        if (encrypted != vec.encrypted) {
            Serial.print("FAIL: RSA encrypt mismatch for input '");
            Serial.print(vec.input.c_str());
            Serial.println("'");
            ok = false;
        }
        if (decrypted != vec.input) {
            Serial.print("FAIL: RSA decrypt mismatch for input '");
            Serial.print(vec.input.c_str());
            Serial.println("'");
            ok = false;
        }
    }

    if (ok) {
        Serial.println("All public self-tests passed.");
    }
    return ok;
}

void runHuntStep() {
    if (gAndrewId.length() == 0) {
        Serial.println("Set your Andrew ID first with: ID <andrewid>");
        return;
    }

    String ssid = chooseBestStationSsid();
    if (ssid.length() == 0) {
        Serial.println("No nearby station APs found.");
        gLastResult = "No nearby stations found";
        return;
    }

    if (!connectToStation(ssid)) {
        gLastResult = "Failed to connect to station AP";
        return;
    }

    String startBody = "{";
    startBody += buildJsonField("andrewId", gAndrewId, true);
    startBody += buildJsonField("boardMac", boardMacAddress(), true);
    startBody += buildJsonField("firmwareVersion", kStudentFirmwareVersion, true);
    startBody += buildJsonField("protocolVersion", 1, false);
    startBody += "}";

    String sessionResponse = postJson(String(kStationBaseUrl) + "/v1/session/start", startBody);
    if (isErrorResponse(sessionResponse)) {
        Serial.print("Session start failed: ");
        Serial.println(sessionResponse);
        gLastResult = sessionResponse;
        return;
    }

    SessionInfo info = parseSessionInfo(sessionResponse);
    Serial.print("Connected to physical station: ");
    Serial.println(info.physicalStationId);
    Serial.print("Logical station: ");
    Serial.println(info.logicalStationId);
    Serial.print("Algorithm: ");
    Serial.println(algorithmToString(info.algorithm).c_str());

    String encryptedResponse = computeEncryptedResponse(info);
    if (encryptedResponse.length() == 0) {
        Serial.println("Unsupported or malformed station challenge.");
        gLastResult = "Unsupported or malformed station challenge";
        return;
    }

    String solveBody = "{";
    solveBody += buildJsonField("sessionId", info.sessionId, true);
    solveBody += buildJsonField("andrewId", gAndrewId, true);
    solveBody += buildJsonField("boardMac", boardMacAddress(), true);
    solveBody += buildJsonField("response", encryptedResponse, false);
    solveBody += "}";

    String solveResponse = postJson(String(kStationBaseUrl) + "/v1/session/solve", solveBody);
    if (isErrorResponse(solveResponse)) {
        Serial.print("Solve request failed: ");
        Serial.println(solveResponse);
        gLastResult = solveResponse;
        return;
    }

    std::string solveJson(solveResponse.c_str());
    String status = extractJsonString(solveJson, "status").c_str();
    String message = extractJsonString(solveJson, "message").c_str();
    String clueCiphertext = extractJsonString(solveJson, "clueCiphertext").c_str();
    String cluePlaintext = clueCiphertext.length() > 0 ? decryptClue(info, clueCiphertext) : message;
    int visited = extractJsonInt(solveJson, "visitedUnique", 0);
    int required = extractJsonInt(solveJson, "required", 0);
    bool complete = extractJsonBool(solveJson, "complete", false);

    Serial.println();
    Serial.print("Status: ");
    Serial.println(status);
    Serial.print("Progress: ");
    Serial.print(visited);
    Serial.print(" / ");
    Serial.println(required);
    Serial.print("Clue: ");
    Serial.println(cluePlaintext);
    if (complete) {
        Serial.println("Completion reached.");
    }
    Serial.println();

    gLastResult = status + ": " + cluePlaintext;
}
}  // namespace

void setup() {
    Serial.begin(115200);
    delay(500);
    preferences.begin("crypto-lab", false);
    loadAndrewId();

    Serial.println();
    Serial.println("Crypto Lab Student Runtime");
    printHelp();
    printStatus();
}

void loop() {
    if (!Serial.available()) {
        delay(20);
        return;
    }

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) {
        return;
    }

    if (line.startsWith("ID ")) {
        saveAndrewId(line.substring(3));
        Serial.print("Saved Andrew ID: ");
        Serial.println(gAndrewId);
        return;
    }
    if (line == "RUN") {
        runHuntStep();
        return;
    }
    if (line == "STATUS") {
        printStatus();
        return;
    }
    if (line == "SELFTEST") {
        runPublicSelfTest();
        return;
    }
    if (line == "HELP") {
        printHelp();
        return;
    }

    Serial.println("Unknown command. Type HELP for options.");
}
