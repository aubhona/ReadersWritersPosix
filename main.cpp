#include <iostream>
#include <pthread.h>
#include <vector>
#include <algorithm>
#include <random>
#include <unistd.h>
#include <string>
#include <fstream>

pthread_mutex_t mutexLog;  // Мьютекс для синхронизации логирования.
pthread_mutex_t mutexCount;  // Мьютекс для синхронизации доступа к счетчику активных потоков.
pthread_cond_t writeFree;  // Условная переменная для оповещения о свободе записи.
pthread_rwlock_t writeLock; // Блокировка для синхронизации писателей.
int readerCount = 0;  // Счетчик активных потоков читатлей.

std::fstream outputFile;  // Файловый поток для вывода логов.

// Глобальные настройки для работы с массивом.
int minArrayElementValue = -100;
int maxArrayElementValue = 100;
int arraySize = 100;

// Количество потоков-читателей и писателей.
int readerThreadCount = 10;
int writerThreadCount = 10;

// Параметры для регулирования задержек потоков.
int minSleepValue = 2;
int maxSleepValue = 7;

// Количество операций чтения/записи, выполняемых каждым потоком.
int readerRuntimeCount = 20;
int writerRuntimeCount = 20;

// Функция для модификации элемента в отсортированном массиве.
void modify(int index, int newValue, std::vector<int>& array) {
    array.erase(array.begin() + index);
    array.insert(std::lower_bound(array.begin(), array.end(), newValue), newValue);
}

// Генерация случайного числа в заданном диапазоне [a, b].
int getRandomNumber(int a, int b) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(a, b);
    return distrib(gen);
}

// Создание и сортировка массива случайных чисел.
std::vector<int> getRandomSortedArray() {
    std::vector<int> array(arraySize);
    for (int i = 0; i < arraySize; ++i) {
        array[i] = getRandomNumber(minArrayElementValue, maxArrayElementValue);
    }
    std::sort(array.begin(), array.end());

    return array;
}

// Функция логирования действий потоков.
void log(std::string&& threadType, int index, std::string&& action) {
    std::cout << "Поток-" << threadType << " №" << index << " " << action << ".\n";
    outputFile << "Поток-" << threadType << " №" << index << " " << action << ".\n";
    outputFile.flush();
}

// Абстрактный класс для потока.
class AbstractThread {
protected:
    pthread_t threadId_{};  // Идентификатор потока.

    static void* thread_func(void* d) {
        (static_cast <AbstractThread*>(d))->run();
        return nullptr;
    }

public:
    virtual ~AbstractThread() {
        end();
    }

    virtual void run() = 0;  // Абстрактный метод, определяющий выполнение потока.

    // Методы для управления жизненным циклом потока.
    int start() {
        return pthread_create(&threadId_, nullptr, AbstractThread::thread_func, this);
    }

    int wait() {
        return pthread_join(threadId_, nullptr);
    }

    int end() {
        return pthread_detach(threadId_);
    }

};

// Класс потока-читателя.
class ReaderThread : public AbstractThread {
protected:
    int id_ = 0;  // Идентификатор потока.
    std::vector<int>* db_ = nullptr;  // Указатель на общий массив данных.

public:

    ReaderThread() = default;

    explicit ReaderThread(int id, std::vector<int>* db) : id_(id), db_(db) {}

    void run() override {
        int count = readerRuntimeCount;
        while (--count >= 0) {
            sleep(getRandomNumber(minSleepValue, maxSleepValue));

            // Блокировка и управление счетчиком активных потоков для объявления о чтении.
            pthread_mutex_lock(&mutexCount);
            ++readerCount;
            pthread_mutex_unlock(&mutexCount);

            // Чтение данных из массива.
            int index = getRandomNumber(0, arraySize - 1);
            int value = db_->at(index);

            // Управление счетчиком активных потоков для объявления о конце чтении
            // и уведомление писателей, если они могут писать.
            pthread_mutex_lock(&mutexCount);
            --readerCount;
            if (readerCount == 0) {
                pthread_cond_signal(&writeFree);
            }
            pthread_mutex_unlock(&mutexCount);

            // Логирование действия потока-читателя.
            pthread_mutex_lock(&mutexLog);
            log("читатель", id_, "прочитал запись под индексом "
                                 + std::to_string(index) + " равную " + std::to_string(value));
            pthread_mutex_unlock(&mutexLog);
        }
    }
};

// Класс потока-писателя.
class WriterThread : public AbstractThread {
protected:
    int id_ = 0;  // Идентификатор потока.
    std::vector<int>* db_ = nullptr;  // Указатель на общий массив данных.

public:

    WriterThread() = default;

    explicit WriterThread(int id, std::vector<int>* db) : id_(id), db_(db) {}

    void run() override {
        int count = writerRuntimeCount;
        while (--count >= 0) {
            sleep(getRandomNumber(minSleepValue, maxSleepValue));

            // Блокировка на запись для писателей.
            pthread_rwlock_wrlock(&writeLock);

            // Блокировка для проверки возможности записи.
            pthread_mutex_lock(&mutexCount);
            while (readerCount > 0) {
                pthread_cond_wait(&writeFree, &mutexCount);
            }
            pthread_mutex_unlock(&mutexCount);

            // Изменение данных в массиве.
            int index = getRandomNumber(0, arraySize - 1);
            int oldValue = db_->at(index);
            int newValue = getRandomNumber(minArrayElementValue, maxArrayElementValue);
            modify(index, newValue, *db_);

            // Разблокировка на запись.
            pthread_rwlock_unlock(&writeLock);

            // Уведомление писателей, если они могут писать.
            pthread_mutex_lock(&mutexCount);
            if (readerCount == 0) {
                pthread_cond_signal(&writeFree);
            }
            pthread_mutex_unlock(&mutexCount);

            // Логирование действия потока-писателя.
            pthread_mutex_lock(&mutexLog);
            log("писатель", id_, "изменил запись под индексом " + std::to_string(index)
                                 + " равную " + std::to_string(oldValue) + ", записав новое значение " +
                                 std::to_string(newValue));
            pthread_mutex_unlock(&mutexLog);
        }
    }
};

// Функция проверки входных параметров.
int checkParam() {
    // Проверка корректности заданных параметров.
    if (minArrayElementValue > maxArrayElementValue || arraySize < 1 || readerThreadCount < 1
        || writerThreadCount < 1 || minSleepValue < 1 || minSleepValue > maxSleepValue || !outputFile.is_open()
        || readerRuntimeCount < 1 || writerRuntimeCount < 1) {
        std::cout << "Некорректные диапазоны входных параметров!\n";
        return 1;
    }

    return 0;
}

// Функция для чтения конфигурационных параметров из файла.
int readConfigFile(char* argv[]) {
    std::fstream inputFile(argv[2], std::ios::in);
    if (!inputFile.is_open()) {
        std::cout << "Ошибка открытия входного файла!\n";
        return 1;
    }
    std::string inputFileName;

    // Чтение параметров из файла.
    inputFile >> minArrayElementValue >> maxArrayElementValue >> arraySize >> readerThreadCount
              >> writerThreadCount >> minSleepValue >> maxSleepValue >> readerRuntimeCount >> writerRuntimeCount
              >> inputFileName;

    outputFile = std::fstream(inputFileName, std::ios::out);

    inputFile.close();
    return checkParam();
}

// Функция для чтения конфигурационных параметров из аргументов командной строки.
int readConfig(int argc, char* argv[]) {
    if (argc == 3 && (std::strcmp(argv[1], "-c") == 0 || std::strcmp(argv[1], "--config") == 0)) {
        return readConfigFile(argv);
    } else if (argc == 12 && (std::strcmp(argv[1], "-p") == 0 || std::strcmp(argv[1], "--params") == 0)) {
        minArrayElementValue = std::stoi(argv[2]);
        maxArrayElementValue = std::stoi(argv[3]);
        arraySize = std::stoi(argv[4]);
        readerThreadCount = std::stoi(argv[5]);
        writerThreadCount = std::stoi(argv[6]);
        minSleepValue = std::stoi(argv[7]);
        maxSleepValue = std::stoi(argv[8]);
        readerRuntimeCount = std::stoi(argv[8]);
        writerRuntimeCount = std::stoi(argv[10]);
        outputFile = std::fstream(argv[11], std::ios::out);
        return checkParam();
    }

    std::cout << "Некорректные аргументы командной строки!\n";
    std::cout << "Введите ключ -c (или --config) и названия файла для чтения из конфигурационного файла\n";
    std::cout << "или введите ключ -p (или --params) и перечислители параметры.\n";
    return 1;
}

// Главная функция программы.
int main(int argc, char* argv[]) {
    // Чтение и проверка конфигурационных параметров.
    if (readConfig(argc, argv) == 1) {
        return 1;
    }

    // Инициализация мьютексов, условной переменной и блокировки.
    pthread_mutex_init(&mutexLog, nullptr);
    pthread_mutex_init(&mutexCount, nullptr);
    pthread_cond_init(&writeFree, nullptr);
    pthread_rwlock_init(&writeLock, nullptr);

    // Создание и сортировка начального массива данных.
    std::vector<int> db = getRandomSortedArray();

    // Создание векторов потоков-читателей и писателей.
    std::vector<ReaderThread> readers(readerThreadCount);
    std::vector<WriterThread> writers(writerThreadCount);

    // Инициализация потоков.
    for (int i = 0; i < readerThreadCount; ++i) {
        readers[i] = ReaderThread(i + 1, &db);
    }

    for (int i = 0; i < writerThreadCount; ++i) {
        writers[i] = WriterThread(i + 1, &db);
    }

    // Запуск потоков.
    for (auto& reader : readers) {
        reader.start();
    }

    for (auto& writer : writers) {
        writer.start();
    }

    // Ожидание завершения работы всех потоков.
    for (auto& reader : readers) {
        reader.wait();
    }

    for (auto& writer : writers) {
        writer.wait();
    }

    // Закрытие файлового потока, уничтожение мьютексов, условной переменной и блокировки.
    outputFile.close();
    pthread_cond_destroy(&writeFree);
    pthread_mutex_destroy(&mutexCount);
    pthread_mutex_destroy(&mutexLog);
    pthread_rwlock_destroy(&writeLock);
}
