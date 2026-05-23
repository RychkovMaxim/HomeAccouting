#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDate>
#include <QString>
#include <vector>

// Предварительное объявление классов виджетов
class QRadioButton;
class QComboBox;
class QDoubleSpinBox;
class QDateEdit;
class QTableWidget;
class QTextEdit;
class QLineEdit;
class QLabel;

// Структура для хранения данных о транзакции
struct Transaction {
    QDate date;
    bool isIncome; // true - доход, false - расход
    QString category;
    double amount;
};

// Структура для записи о цене товара в конкретном магазине
struct CatalogItem {
    QString productName;
    QString storeName;
    double price;
};

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateCategories();
    void addTransaction();
    void generateReport();
    void deleteTransaction(); // Слот для удаления транзакции
    void deleteCatalogItem(); // Слот для удаления товара из каталога

private:
    // Методы создания интерфейса
    void setupUI();
    QWidget* createInputTab();
    QWidget* createHistoryTab();
    QWidget* createAnalysisTab();
    QWidget* createReportTab();

    // Методы обновления данных
    void refreshViews();
    void updateHistoryTable();
    void updateAnalysisData();

    // Хранилище данных
    std::vector<Transaction> transactions;

    // Элементы UI (Блок 1: Пользовательский ввод)
    QRadioButton* rbIncome;
    QRadioButton* rbExpense;
    QComboBox* cbCategory;
    QDoubleSpinBox* sbAmount;
    QDateEdit* dateEdit;

    // Элементы UI (История)
    QTableWidget* tableHistory;

    // Элементы UI (Блок 3: Анализ)
    QTableWidget* tableAnalysis;
    QTextEdit* textAnomalies;

    // Элементы UI (Блок 4: Отчет и план)
    QLineEdit* editGoalName;
    QDoubleSpinBox* sbGoalAmount;
    QTextEdit* textReport;

    //Работа с файлом
    void saveToFile();   // Запись данных на диск
    void loadFromFile(); // Чтение данных при старте

    std::vector<CatalogItem> catalogItems; // База данных каталога

    QLineEdit* editProdName;
    QComboBox* cbStoreName;
    QDoubleSpinBox* sbProdPrice;
    QTableWidget* tableCatalog;
    QLabel* lblCompareResult;

    // Методы
    QWidget* createCatalogTab();
    void addCatalogItem();
    void comparePrices();
    void saveCatalogToFile();
    void loadCatalogFromFile();
};

#endif // MAINWINDOW_H