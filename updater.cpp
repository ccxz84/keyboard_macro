#include <iostream>
#include <curl/curl.h>
#include <json/json.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

// 플랫폼 및 Bluetooth 지원 매크로 정의
#if defined(__aarch64__)
#define PLATFORM "ARM 64bit"
#define BIT_SUFFIX "_64bit"
#elif defined(__arm__)
#define PLATFORM "ARM 32bit"
#define BIT_SUFFIX "_32bit"
#else
#error "Unsupported platform"
#endif

// Bluetooth 지원 여부 매크로 정의 (필요에 따라 설정)
#if defined(HAVE_BLUETOOTH)
#define BLUETOOTH "Bluetooth"
#define BLUETOOTH_SUFFIX "_bluetooth"
#else
#define BLUETOOTH "No Bluetooth"
#define BLUETOOTH_SUFFIX "_nonbluetooth"
#endif

size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string getLatestVersionFromGitHub() {
    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/ccxz84/keyboard_macro/releases/latest");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.68.0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    Json::Value jsonData;
    Json::Reader jsonReader;
    if (jsonReader.parse(readBuffer, jsonData)) {
        return jsonData["tag_name"].asString();
    }

    return "";
}

std::string getCurrentVersion() {
    std::ifstream versionFile("version.txt");
    std::string version;
    std::getline(versionFile, version);
    return version;
}

size_t WriteFileCallback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

void downloadUpdateFile(const std::string &url, const std::string &outputPath) {
    CURL *curl;
    FILE *fp;
    CURLcode res;
    long http_code = 0;

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(outputPath.c_str(), "wb");
        if (!fp) {
            std::cerr << "Failed to open file: " << outputPath << std::endl;
            return;
        }

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);  // Fail on HTTP errors
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  // Follow redirects

        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        } else if(http_code != 200) {
            std::cerr << "HTTP request failed with code: " << http_code << std::endl;
        } else {
            std::cout << "Download succeeded: " << url << std::endl;
        }

        curl_easy_cleanup(curl);
        fclose(fp);
    } else {
        std::cerr << "Failed to initialize CURL" << std::endl;
    }
}

bool verifyDownloadedFile(const std::string &filePath, const std::string &expectedHash) {
    // 해시 검증 로직을 여기에 추가하세요.
    return true; // 예시로 true 반환
}

void installUpdate(const std::string &updateFilePath) {
    const std::string targetDirectory = "/home/ccxz84";
    const std::string targetFilePath = targetDirectory + "/KeyboardDriver"; // 변경할 타겟 파일 이름

    try {
        // 타겟 디렉토리가 존재하는지 확인하고, 없으면 생성
        if (!fs::exists(targetDirectory)) {
            fs::create_directories(targetDirectory);
        }

        // 다운로드한 파일을 타겟 디렉토리로 복사
        fs::copy_file(updateFilePath, targetFilePath, fs::copy_options::overwrite_existing);

        // 실행 권한 부여
        fs::permissions(targetFilePath, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec, fs::perm_options::add);

        std::cout << "Update installed successfully to " << targetFilePath << std::endl;
    } catch (const fs::filesystem_error &e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "General error: " << e.what() << std::endl;
    }
}

void performUpdate() {
    std::string currentVersion = getCurrentVersion();
    std::string latestVersion = getLatestVersionFromGitHub();

    if (latestVersion != currentVersion) {
        std::string updateFileName = "KeyboardDriver" + std::string(BIT_SUFFIX) + std::string(BLUETOOTH_SUFFIX);
        std::string updateUrl = "https://github.com/ccxz84/keyboard_macro/releases/download/" + latestVersion + "/" + updateFileName;
        std::cout<<updateUrl<<'\n';
        std::string updateFilePath = "/home/ccxz84/update";

        downloadUpdateFile(updateUrl, updateFilePath);

        if (verifyDownloadedFile(updateFilePath, "expected_hash_value")) {
            installUpdate(updateFilePath);
            std::ofstream versionFile("version.txt");
            versionFile << latestVersion;
        } else {
            std::cerr << "Downloaded file verification failed!" << std::endl;
        }
    } else {
        std::cout << "No updates available." << std::endl;
    }
}

int main() {
    std::string currentVersion = getCurrentVersion();
    std::cout << "App Version: " << currentVersion << std::endl;
    std::cout << "Platform: " << PLATFORM << std::endl;
    std::cout << "Bluetooth: " << BLUETOOTH << std::endl;

    performUpdate();

    return 0;
}
