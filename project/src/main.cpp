// Каркас агента: читает журнал событий построчно и считает строки.
//
// Это заготовка занятия 1.1, а не решение. Детектов она не ищет — их вы
// добавите здесь же, в отмеченном месте ниже. Формат строки детекта, список
// признаков и правило про их порядок заданы в постановке занятия: по ним
// сравниваются эталоны.
//
// Весь код лежит в main, и на этом занятии так и надо: функции появятся
// на занятии 1.2, ссылки — на 1.3. Разбор аргументов, коды возврата и флаг
// --quiet — часть задания.
//
// Запуск:
//   nano-edr <журнал.log>
#include <cstdio>
#include <fstream>
#include <print>
#include <string>
#include <map>
#include <vector>

int main(int argc, char** argv) {
    // Аргументы разбираются грубо: путь к журналу и ничего больше. Остальное,
    // включая --quiet, добавляется по заданию.
    std::string path;
    bool quiet = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--quiet") {
            quiet = true;
        } else if (path.empty()) {
            path = arg;
        }
    }
    if (path.empty()) {
        std::print(stderr, "использование: nano-edr <журнал.log> [--quiet]\n");
        return 2;
    }
    std::ifstream log(path);
    if (!log) {
        std::print(stderr, "не удалось открыть журнал {}\n", path);
        return 2;
    }

    long long lines = 0;
    std::map<std::string, long long> type_counts;
    std::string line;
    const std::vector<std::string> signs = {"wscript.exe", ".locked", "certutil.exe", "\\Startup\\"};

    while (std::getline(log, line)) {
        // Счётчик увеличивается до всех проверок: он считает строки файла,
        // а не события. Номер, посчитанный по событиям, бесполезен — по нему
        // нельзя открыть файл и посмотреть.
        ++lines;

        // Строки-комментарии в журнале начинаются с '#'. Они не события,
        // и детекта по ним быть не должно.
        

        // >>> Здесь начинается занятие 1.1.
        std::size_t i = 0;
        while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) {
            ++i;
        }
        if (i == line.size()) {
            continue;
        }
        if (line[i] == '#' || line[i] == ';') {
            continue;
        }
        std::size_t tpos = line.find("type=");
        if (tpos != std::string::npos) {
            std::size_t tstart = tpos + 5;
            std::size_t tend = line.find(' ', tstart);
            std::string type = (tend == std::string::npos) ? line.substr(tstart) : line.substr(tstart, tend -tstart);
            ++type_counts[type];
        }
        for (const std::string& sign : signs) {
            if (line.find(sign) != std::string::npos) {
                std::print("[DETECT] строка {}, признак {}: {}\n", lines, sign, line);
            }
        }
        // Проверка признаков и печать детекта. Номер строки, который нужен
        // в выводе, — это lines.
    }
    if (!quiet) {
        std::print("строк {} всего\n", lines);
        std::print("события по типам:\n");
        for (const auto& [type, count] : type_counts) {
            std::print(" {}: {}\n", type, count);
        }
    }
    return 0;
}
