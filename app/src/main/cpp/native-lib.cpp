#include <jni.h>
#include <string>
#include <fstream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <android/log.h>
#include <ctime>
#include <chrono>
#include <strstream>
#include <regex>
#include <iomanip>

long long timeDifferenceInMillis = 0;
long long firstEventTimeInMillis = 0;

std::string removeSpaces(const std::string& str) {
    std::string result;
    for (char ch : str) {
        if (!std::isspace(ch)) {
            result += ch;
        }
    }
    return result;
}


bool isDateTimeInRangeToActiveTime(const std::string& dateTime, const char* startRange, const char* endRange) {
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "Original DateTime: %s", dateTime.c_str());

    // Remover espaços em branco
    std::string dateTimeNoSpaces = removeSpaces(dateTime);
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "Before removeSpaces: %s", dateTimeNoSpaces.c_str());

    // Modificar a lógica para extrair corretamente a data/hora
    std::istringstream dateStream(dateTimeNoSpaces);
    int hour, minute, second;
    char colon, period;

    dateStream >> hour >> colon >> minute >> colon >> second >> period;

    if (period == 'P' || period == 'p') {
        // Se o período é PM, adicione 12 horas
        if (hour != 12) {
            hour += 12;
        }
    } else if (period == 'A' || period == 'a') {
        // Se o período é AM e a hora é 12, defina para 0
        if (hour == 12) {
            hour = 0;
        }
    }

    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "After parsing: %02d:%02d:%02d", hour, minute, second);

    // Adicionando a lógica para comparar com os intervalos de início e fim
    struct tm tmDateTime = {};
    tmDateTime.tm_hour = hour;
    tmDateTime.tm_min = minute;
    tmDateTime.tm_sec = second;

    struct tm tmStart = {};
    sscanf(startRange, "%d:%d:%d", &tmStart.tm_hour, &tmStart.tm_min, &tmStart.tm_sec);

    struct tm tmEnd = {};
    sscanf(endRange, "%d:%d:%d", &tmEnd.tm_hour, &tmEnd.tm_min, &tmEnd.tm_sec);

    // Remova a informação de data da struct tm (definindo todos os campos para zero)
    tmDateTime.tm_year = tmStart.tm_year = tmEnd.tm_year = 0;
    tmDateTime.tm_mon = tmStart.tm_mon = tmEnd.tm_mon = 0;
    tmDateTime.tm_mday = tmStart.tm_mday = tmEnd.tm_mday = 0;

    // Ajuste para garantir que as comparações estejam corretas
    if (tmDateTime.tm_hour < tmStart.tm_hour ||
        (tmDateTime.tm_hour == tmStart.tm_hour && tmDateTime.tm_min < tmStart.tm_min) ||
        (tmDateTime.tm_hour == tmStart.tm_hour && tmDateTime.tm_min == tmStart.tm_min && tmDateTime.tm_sec < tmStart.tm_sec)) {
        // A data/hora está antes do início do intervalo
        return false;
    }

    if (tmDateTime.tm_hour > tmEnd.tm_hour ||
        (tmDateTime.tm_hour == tmEnd.tm_hour && tmDateTime.tm_min > tmEnd.tm_min) ||
        (tmDateTime.tm_hour == tmEnd.tm_hour && tmDateTime.tm_min == tmEnd.tm_min && tmDateTime.tm_sec > tmEnd.tm_sec)) {
        // A data/hora está após o final do intervalo
        return false;
    }

    // O evento está dentro do intervalo
    // Calcular a diferença em milissegundos
    long long currentEventTimeInMillis = (hour * 3600 + minute * 60 + second) * 1000;
    if (firstEventTimeInMillis == 0) {
        firstEventTimeInMillis = currentEventTimeInMillis;
    }

    timeDifferenceInMillis = currentEventTimeInMillis - firstEventTimeInMillis;

    return true;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_projetomestrado_MainActivity_getTotalActiveTimeJNI(
        JNIEnv* env,
        jobject /* this */,
        jstring controllerID,
        jstring startTime,
        jstring endTime) {

    const char *controllerIDChars = env->GetStringUTFChars(controllerID, 0);
    const char *startTimeChars = env->GetStringUTFChars(startTime, 0);
    const char *endTimeChars = env->GetStringUTFChars(endTime, 0);

    __android_log_print(ANDROID_LOG_ERROR, "getTotal", "xxx controllerID: %s", controllerIDChars);
    __android_log_print(ANDROID_LOG_ERROR, "getTotal", "xxx startTimeChars: %s", startTimeChars);
    __android_log_print(ANDROID_LOG_ERROR, "getTotal", "xxx endTimeChars: %s", endTimeChars);

    int targetControllerID = std::atoi(controllerIDChars);
    std::chrono::steady_clock::time_point lastEventTime;
    std::chrono::steady_clock::duration controllerTotalTime;

    const char *filePath = "/sdcard/Download/lista3.csv";

    std::ifstream file(filePath);
    if (!file.is_open()) {
        __android_log_print(ANDROID_LOG_ERROR, "getTotal", "Error opening file: %s", filePath);
        return env->NewStringUTF("Error opening file");
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t controllerPos = line.find("ID do Controlador: ");
        if (controllerPos != std::string::npos) {
            size_t datePos = line.find("Data/Hora: ");
            if (datePos != std::string::npos) {
                std::string controllerIDStr = line.substr(controllerPos + 19, line.find(",", controllerPos) - (controllerPos + 19));
                int currentControllerID = std::stoi(controllerIDStr);

                if (currentControllerID == targetControllerID) {
                    std::string dateTime = line.substr(datePos + 11);
                    if (isDateTimeInRangeToActiveTime(dateTime, startTimeChars, endTimeChars)) {
                        // Se o evento está dentro do intervalo, marque o controlador como ativo
                        __android_log_print(ANDROID_LOG_DEBUG, "getTotal", "timeDifferenceInMillis: %lld", timeDifferenceInMillis);
                    } else {
                        // Se o evento está fora do intervalo, marque o controlador como inativo
                        __android_log_print(ANDROID_LOG_DEBUG, "getTotal", "Fora do intervalo",
                                            targetControllerID, dateTime.c_str());
                    }
                }
            }
        }
    }

    file.close();

    env->ReleaseStringUTFChars(controllerID, controllerIDChars);
    env->ReleaseStringUTFChars(startTime, startTimeChars);
    env->ReleaseStringUTFChars(endTime, endTimeChars);

    // Converter a diferença de tempo para horas, minutos e segundos
    long long hours = timeDifferenceInMillis / 3600000;
    long long minutes = (timeDifferenceInMillis % 3600000) / 60000;
    long long seconds = (timeDifferenceInMillis % 60000) / 1000;

    // Construir uma string que contém o tempo total para o controlador específico
    std::stringstream result;
    result << "ID do Controlador: " << targetControllerID
           << ", Tempo Total Ativo: " << hours << " horas, " << minutes << " minutos, " << seconds << " segundos";

    __android_log_print(ANDROID_LOG_DEBUG, "getTotal", "Total Active Time for controller %d: %s", targetControllerID, result.str().c_str());

    return env->NewStringUTF(result.str().c_str());
}

bool isDateTimeInRange(const std::string& dateTime, const char* startRange, const char* endRange) {
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "Original DateTime: %s", dateTime.c_str());

    // Remover espaços em branco
    std::string dateTimeNoSpaces = removeSpaces(dateTime);
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "Before removeSpaces: %s", dateTimeNoSpaces.c_str());

    // Modificar a lógica para extrair corretamente a data/hora
    std::istringstream dateStream(dateTimeNoSpaces);
    int hour, minute, second;
    char colon, period;

    dateStream >> hour >> colon >> minute >> colon >> second >> period;

    if (period == 'P' || period == 'p') {
        // Se o período é PM, adicione 12 horas
        if (hour != 12) {
            hour += 12;
        }
    } else if (period == 'A' || period == 'a') {
        // Se o período é AM e a hora é 12, defina para 0
        if (hour == 12) {
            hour = 0;
        }
    }

    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "After parsing: %02d:%02d:%02d", hour, minute, second);

    // Adicionando a lógica para comparar com os intervalos de início e fim
    struct tm tmDateTime = {};
    tmDateTime.tm_hour = hour;
    tmDateTime.tm_min = minute;
    tmDateTime.tm_sec = second;

    struct tm tmStart = {};
    sscanf(startRange, "%d:%d:%d", &tmStart.tm_hour, &tmStart.tm_min, &tmStart.tm_sec);

    struct tm tmEnd = {};
    sscanf(endRange, "%d:%d:%d", &tmEnd.tm_hour, &tmEnd.tm_min, &tmEnd.tm_sec);

    // Remova a informação de data da struct tm (definindo todos os campos para zero)
    tmDateTime.tm_year = tmStart.tm_year = tmEnd.tm_year = 0;
    tmDateTime.tm_mon = tmStart.tm_mon = tmEnd.tm_mon = 0;
    tmDateTime.tm_mday = tmStart.tm_mday = tmEnd.tm_mday = 0;

    // Ajuste para garantir que as comparações estejam corretas
    if (tmDateTime.tm_hour < tmStart.tm_hour ||
        (tmDateTime.tm_hour == tmStart.tm_hour && tmDateTime.tm_min < tmStart.tm_min) ||
        (tmDateTime.tm_hour == tmStart.tm_hour && tmDateTime.tm_min == tmStart.tm_min && tmDateTime.tm_sec < tmStart.tm_sec)) {
        // A data/hora está antes do início do intervalo
        return false;
    }

    if (tmDateTime.tm_hour > tmEnd.tm_hour ||
        (tmDateTime.tm_hour == tmEnd.tm_hour && tmDateTime.tm_min > tmEnd.tm_min) ||
        (tmDateTime.tm_hour == tmEnd.tm_hour && tmDateTime.tm_min == tmEnd.tm_min && tmDateTime.tm_sec > tmEnd.tm_sec)) {
        // A data/hora está após o final do intervalo
        return false;
    }

    // O evento está dentro do intervalo
    return true;
}



extern "C" JNIEXPORT jstring JNICALL
Java_com_example_projetomestrado_MainActivity_readCsvFile(
        JNIEnv* env,
        jobject /* this */) {

    const char *filePath = "/sdcard/Download/lista.csv";

    if (access(filePath, F_OK) == -1) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", "File does not exist: %s", filePath);
        return env->NewStringUTF("File does not exist");
    }

    std::ifstream file(filePath);
    if (!file.is_open()) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", "Error opening file: %s", filePath);
        return env->NewStringUTF("Error opening file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    file.close();

    __android_log_print(ANDROID_LOG_DEBUG, "MyApp", "File content: %s", content.c_str());

    return env->NewStringUTF(content.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_projetomestrado_MainActivity_listEventsInRangeJNI(
        JNIEnv* env,
        jobject /* this */,
        jstring startTime,
        jstring endTime) {

    const char *startTimeChars = env->GetStringUTFChars(startTime, 0);
    const char *endTimeChars = env->GetStringUTFChars(endTime, 0);

    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "xxx startTimeChars: %s", startTimeChars);
    __android_log_print(ANDROID_LOG_ERROR, "MyApp", "xxx endTimeChars: %s", endTimeChars);

    // Lógica para processar eventos no intervalo de datas
    std::stringstream eventsInRange;

    const char *filePath = "/sdcard/Download/lista4.csv";

    std::ifstream file(filePath);
    if (!file.is_open()) {
        __android_log_print(ANDROID_LOG_ERROR, "MyApp", "Error opening file: %s", filePath);
        return env->NewStringUTF("Error opening file");
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t datePos = line.find("Data/Hora: ");
        if (datePos != std::string::npos) {
            std::string dateTime = line.substr(datePos + 11); // Adicionado offset para pegar a data/hora correta
            __android_log_print(ANDROID_LOG_DEBUG, "MyApp", "DateTime: %s", dateTime.c_str());

            // Declarar tmDateTime dentro deste escopo
            if (isDateTimeInRange(dateTime, startTimeChars, endTimeChars)) {
                eventsInRange << line << "\n";
            }
        }
    }

    file.close();

    env->ReleaseStringUTFChars(startTime, startTimeChars);
    env->ReleaseStringUTFChars(endTime, endTimeChars);

    __android_log_print(ANDROID_LOG_DEBUG, "MyApp", "Events in range: %s", eventsInRange.str().c_str());

    return env->NewStringUTF(eventsInRange.str().c_str());
}
