Eng:

The task of readers and writers ("confirmed reading"). The database is divided into two types of processes — readers and writers. Readers periodically look through random database entries and output their number, record index and its value. Writers change random entries to a random number and also display information about their number, record index, old value and new value. It is assumed that the database is in a consistent state at the beginning (all numbers are sorted). Each individual new record transfers the database from one consistent state to another (that is, the new sorting can change the indexes of records). Transactions are performed in the "confirmed read" mode, that is, the writer process cannot access the database if it is occupied by another writer process or reader process. Any number of reader processes can access the database at the same time. The reader process gets access to the database, even if it has already been occupied by the writer process. Create a multi-thread application with writer threads and reader threads.

It was solved using Posix Thread C++
__________

Rus:

Задача о читателях и писателях («подтвержденное чтение»). Базу данных разделяют два типа процессов — читатели и писатели. Чи- татели периодически просматривают случайные записи базы дан- ных и выводя номер свой номер, индекс записи и ее значение. Писа- тели изменяют случайные записи на случайное число и также выво- дят информацию о своем номере, индексе записи, старом значении и новом значении. Предполагается, что в начале БД находится в непротиворечивом состоянии (все числа отсортированы). Каждая отдельная новая запись переводит БД из одного непротиворечиво- го состояния в другое (то есть, новая сортировка может поменять индексы записей). Транзакции выполняются в режиме «подтвер- жденного чтения», то есть процесс-писатель не может получить до- ступ к БД в том случае, если ее занял другой процесс–писатель или процесс–читатель. К БД может обратиться одновременно сколько угодно процессов–читателей. Процесс читатель получает доступ к БД, даже если ее уже занял процесс–писатель. Создать многопо- точное приложение с потоками–писателями и потоками–читателями.

Было решено с использованием Posix Thread C++
