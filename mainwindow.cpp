#include "mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QtWidgets>
#include <map>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Домашняя бухгалтерия");
    setMinimumSize(800, 600);

    // Применяем современный стиль интерфейса
    setStyleSheet(R"(
    QMainWindow { background-color: #f4f6f9; }
    QTabWidget::pane { border: 1px solid #dcdde1; background-color: #ffffff; border-radius: 6px; }
    QTabBar::tab { background: #e1e2e6; padding: 12px 25px; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; font-size: 14px; color: #2f3640; }
    QTabBar::tab:selected { background: #ffffff; font-weight: bold; color: #2f3640; border-bottom: 2px solid #00a8ff; }

    /* Общий стиль текста для меток и кнопок выбора */
    QLabel, QRadioButton, QGroupBox {
        color: #2f3640;
        font-size: 13px;
    }

    /* Поля ввода */
    QLineEdit, QDoubleSpinBox, QDateEdit {
        padding: 8px;
        border: 1px solid #dcdde1;
        border-radius: 4px;
        background: #ffffff;
        color: #2f3640;
    }

    /* Выпадающий список (ComboBox) */
    QComboBox {
        padding: 8px;
        border: 1px solid #dcdde1;
        border-radius: 4px;
        background: #ffffff;
        color: #2f3640;
    }

    /* Цвет текста ВНУТРИ выпадающего списка */
    QComboBox QAbstractItemView {
        background-color: #ffffff;
        color: #2f3640;
        selection-background-color: #00a8ff;
        selection-color: #ffffff;
        border: 1px solid #dcdde1;
    }

    QPushButton { background-color: #00a8ff; color: white; border: none; padding: 10px 20px; border-radius: 5px; font-weight: bold; }
    QPushButton:hover { background-color: #0097e6; }

    QTableWidget { gridline-color: #f1f2f6; border: 1px solid #dcdde1; color: #2f3640; }
    QHeaderView::section { background-color: #f5f6fa; padding: 8px; color: #2f3640; }
)");

    setupUI();
    loadFromFile();
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI() {
    QTabWidget* tabs = new QTabWidget(this);
    setCentralWidget(tabs);

    tabs->addTab(createInputTab(), "Ввод данных");
    tabs->addTab(createHistoryTab(), "История транзакций");
    tabs->addTab(createAnalysisTab(), "Анализ трат");
    tabs->addTab(createReportTab(), "Отчет и планирование");
}

QWidget* MainWindow::createInputTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    QGroupBox* group = new QGroupBox("Новая операция");
    QFormLayout* form = new QFormLayout(group);
    form->setSpacing(15);

    // Тип операции
    QHBoxLayout* typeLayout = new QHBoxLayout();
    rbExpense = new QRadioButton("Расход");
    rbIncome = new QRadioButton("Доход");
    rbExpense->setChecked(true);
    typeLayout->addWidget(rbExpense);
    typeLayout->addWidget(rbIncome);
    form->addRow("Тип:", typeLayout);

    // Категория (Классификатор)
    cbCategory = new QComboBox();
    updateCategories();
    connect(rbExpense, &QRadioButton::toggled, this, &MainWindow::updateCategories);
    connect(rbIncome, &QRadioButton::toggled, this, &MainWindow::updateCategories);
    form->addRow("Категория:", cbCategory);

    // Сумма
    sbAmount = new QDoubleSpinBox();
    sbAmount->setMaximum(1000000000);
    sbAmount->setDecimals(2);
    sbAmount->setSuffix(" ₽");
    form->addRow("Сумма операции:", sbAmount);

    // Дата
    dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    form->addRow("Дата:", dateEdit);

    // Кнопка добавления
    QPushButton* btnAdd = new QPushButton("Добавить транзакцию");
    connect(btnAdd, &QPushButton::clicked, this, &MainWindow::addTransaction);

    layout->addWidget(group);
    layout->addWidget(btnAdd, 0, Qt::AlignCenter);
    layout->addStretch();

    return tab;
}

QWidget* MainWindow::createHistoryTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    tableHistory = new QTableWidget(0, 4);
    tableHistory->setHorizontalHeaderLabels({"Дата", "Тип", "Категория", "Сумма"});
    tableHistory->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableHistory->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(tableHistory);

    return tab;
}

QWidget* MainWindow::createAnalysisTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // Статистика категорий
    QGroupBox* groupStats = new QGroupBox("Статистика по категориям");
    QVBoxLayout* lStats = new QVBoxLayout(groupStats);
    tableAnalysis = new QTableWidget(0, 2);
    tableAnalysis->setHorizontalHeaderLabels({"Категория", "Итоговая сумма"});
    tableAnalysis->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableAnalysis->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lStats->addWidget(tableAnalysis);

    // Детектор аномалий и модуль сравнения
    QGroupBox* groupAnomalies = new QGroupBox("Детектор аномалий");
    QVBoxLayout* lAnomalies = new QVBoxLayout(groupAnomalies);
    textAnomalies = new QTextEdit();
    textAnomalies->setReadOnly(true);
    lAnomalies->addWidget(textAnomalies);

    layout->addWidget(groupStats, 1);
    layout->addWidget(groupAnomalies, 1);

    return tab;
}

QWidget* MainWindow::createReportTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* groupGoal = new QGroupBox("Финансовая цель");
    QFormLayout* form = new QFormLayout(groupGoal);
    editGoalName = new QLineEdit();
    editGoalName->setPlaceholderText("Например: Отпуск на море");
    sbGoalAmount = new QDoubleSpinBox();
    sbGoalAmount->setMaximum(1000000000);
    sbGoalAmount->setSuffix(" ₽");

    form->addRow("Название цели:", editGoalName);
    form->addRow("Требуемая сумма:", sbGoalAmount);

    QPushButton* btnReport = new QPushButton("Сгенерировать отчет и план");
    btnReport->setObjectName("btnReport");
    connect(btnReport, &QPushButton::clicked, this, &MainWindow::generateReport);

    textReport = new QTextEdit();
    textReport->setReadOnly(true);

    layout->addWidget(groupGoal);
    layout->addWidget(btnReport, 0, Qt::AlignCenter);
    layout->addWidget(textReport, 1);

    return layout->parentWidget();
}

void MainWindow::updateCategories() {
    cbCategory->clear();
    if (rbIncome->isChecked()) {
        cbCategory->addItems({"Зарплата", "Премия", "Подарки", "Инвестиции", "Прочее"});
    } else {
        cbCategory->addItems({"Продукты", "Транспорт", "ЖКХ", "Развлечения", "Здоровье", "Одежда", "Прочее"});
    }
}

void MainWindow::addTransaction() {
    if (sbAmount->value() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Сумма должна быть больше нуля.");
        return;
    }

    Transaction t;
    t.date = dateEdit->date();
    t.isIncome = rbIncome->isChecked();
    t.category = cbCategory->currentText();
    t.amount = sbAmount->value();

    transactions.push_back(t);

    sbAmount->setValue(0);
    QMessageBox::information(this, "Успех", "Операция успешно добавлена!");

    refreshViews();
    saveToFile();
}

void MainWindow::refreshViews() {
    updateHistoryTable();
    updateAnalysisData();
}

void MainWindow::updateHistoryTable() {
    tableHistory->setRowCount(static_cast<int>(transactions.size()));
    for (size_t i = 0; i < transactions.size(); ++i) {
        const auto& t = transactions[i];
        tableHistory->setItem(i, 0, new QTableWidgetItem(t.date.toString("dd.MM.yyyy")));
        tableHistory->setItem(i, 1, new QTableWidgetItem(t.isIncome ? "Доход" : "Расход"));
        tableHistory->setItem(i, 2, new QTableWidgetItem(t.category));

        QTableWidgetItem* amountItem = new QTableWidgetItem(QString::number(t.amount, 'f', 2) + " ₽");
        amountItem->setForeground(QBrush(t.isIncome ? QColor("#27ae60") : QColor("#c0392b")));
        tableHistory->setItem(i, 3, amountItem);
    }
}

void MainWindow::updateAnalysisData() {
    std::map<QString, double> expenseByCategory;
    double totalExpense = 0.0;

    for (const auto& t : transactions) {
        if (!t.isIncome) {
            expenseByCategory[t.category] += t.amount;
            totalExpense += t.amount;
        }
    }

    tableAnalysis->setRowCount(static_cast<int>(expenseByCategory.size()));
    int row = 0;
    for (const auto& pair : expenseByCategory) {
        tableAnalysis->setItem(row, 0, new QTableWidgetItem(pair.first));
        tableAnalysis->setItem(row, 1, new QTableWidgetItem(QString::number(pair.second, 'f', 2) + " ₽"));
        row++;
    }

    QString anomaliesText = "<b>Анализ текущих трат:</b><br>";
    if (totalExpense == 0) {
        textAnomalies->setText("Нет данных о расходах для анализа.");
        return;
    }

    bool anomalyFound = false;
    for (const auto& pair : expenseByCategory) {
        double percentage = (pair.second / totalExpense) * 100.0;
        if (percentage > 40.0 && pair.first != "ЖКХ" && pair.first != "Продукты") {
            anomaliesText += QString("<span style='color:#e74c3c;'>Внимание!</span> Траты на '<b>%1</b>' составляют %2% от всех расходов. "
                                     "Рекомендуется пересмотреть бюджет в этой категории.<br><br>")
                                 .arg(pair.first).arg(percentage, 0, 'f', 1);
            anomalyFound = true;
        }
    }

    if (!anomalyFound) {
        anomaliesText += "<span style='color:#27ae60;'>Бюджет выглядит сбалансированным. Аномальных перекосов в тратах не выявлено.</span>";
    }

    textAnomalies->setHtml(anomaliesText);
}

void MainWindow::generateReport() {
    double totalIncome = 0;
    double totalExpense = 0;

    for (const auto& t : transactions) {
        if (t.isIncome) totalIncome += t.amount;
        else totalExpense += t.amount;
    }

    double balance = totalIncome - totalExpense;

    QString report = "<h2>Финансовый отчет</h2>";
    report += QString("<b>Общие доходы:</b> <span style='color:#27ae60;'>%1 ₽</span><br>").arg(totalIncome, 0, 'f', 2);
    report += QString("<b>Общие расходы:</b> <span style='color:#c0392b;'>%1 ₽</span><br>").arg(totalExpense, 0, 'f', 2);
    report += QString("<b>Свободный остаток:</b> %1 ₽<br><br>").arg(balance, 0, 'f', 2);

    QString goalName = editGoalName->text();
    double goalAmount = sbGoalAmount->value();

    report += "<h2>Финансовый план</h2>";
    if (goalAmount > 0) {
        if (balance <= 0) {
            report += "<span style='color:#e74c3c;'>К сожалению, текущий свободный остаток не позволяет делать накопления. "
                      "Рекомендуем сократить расходы, опираясь на вкладку «Анализ трат».</span>";
        } else {
            double monthsNeeded = goalAmount / balance;
            int months = std::ceil(monthsNeeded);
            report += QString("Вы копите на: <b>%1</b> (Сумма: %2 ₽)<br>")
                          .arg(goalName.isEmpty() ? "Неизвестная цель" : goalName)
                          .arg(goalAmount, 0, 'f', 2);
            report += QString("При сохранении текущей динамики свободных средств, вы достигнете цели примерно через <b>%1 месяцев</b>.<br>")
                          .arg(months);
        }
    } else {
        report += "<i>Финансовая цель не задана. Укажите цель для формирования плана накоплений.</i>";
    }

    textReport->setHtml(report);
}
// Метод для записи всех транзакций в файл
void MainWindow::saveToFile() {
    QFile file("transactions.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const auto& t : transactions) {
            // Записываем через разделитель (например, точка с запятой)
            out << t.date.toString("yyyy-MM-dd") << ";"
                << (t.isIncome ? "1" : "0") << ";"
                << t.category << ";"
                << t.amount << "\n";
        }
        file.close();
    }
}

// Метод для чтения данных при запуске
void MainWindow::loadFromFile() {
    QFile file("transactions.txt");
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(";");
            if (parts.size() == 4) {
                Transaction t;
                t.date = QDate::fromString(parts[0], "yyyy-MM-dd");
                t.isIncome = (parts[1] == "1");
                t.category = parts[2];
                t.amount = parts[3].toDouble();
                transactions.push_back(t);
            }
        }
        file.close();
        refreshViews(); // Обновляем таблицы сразу после загрузки
    }
}