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
    tabs->addTab(createCatalogTab(), "Каталог и сравнение");
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

    QPushButton* btnDeleteTrans = new QPushButton("Удалить выбранную транзакцию");
    btnDeleteTrans->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold;"); // Сделаем её аккуратно красной
    connect(btnDeleteTrans, &QPushButton::clicked, this, &MainWindow::deleteTransaction);

    layout->addWidget(tableHistory);
    layout->addWidget(btnDeleteTrans);

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
        refreshViews();
    }
}

QWidget* MainWindow::createCatalogTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* mainLayout = new QHBoxLayout(tab);

    // Левая часть: Форма ввода
    QGroupBox* boxInput = new QGroupBox("Добавить цену в каталог");
    QFormLayout* form = new QFormLayout(boxInput);

    editProdName = new QLineEdit();
    editProdName->setPlaceholderText("Например: Молоко 1л");

    cbStoreName = new QComboBox();
    cbStoreName->addItems({"Пятёрочка", "Магнит", "Перекрёсток", "Лента", "ВкусВилл"});

    sbProdPrice = new QDoubleSpinBox();
    sbProdPrice->setMaximum(100000);
    sbProdPrice->setSuffix(" ₽");

    QPushButton* btnAddCatalog = new QPushButton("Внести в каталог");
    connect(btnAddCatalog, &QPushButton::clicked, this, &MainWindow::addCatalogItem);

    form->addRow("Товар:", editProdName);
    form->addRow("Магазин:", cbStoreName);
    form->addRow("Цена:", sbProdPrice);
    form->addRow(btnAddCatalog);

    QVBoxLayout* rightLayout = new QVBoxLayout();

    tableCatalog = new QTableWidget(0, 3);
    tableCatalog->setHorizontalHeaderLabels({"Товар", "Магазин", "Цена"});
    tableCatalog->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableCatalog->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QPushButton* btnDeleteCatalog = new QPushButton("Удалить выбранный товар");
    btnDeleteCatalog->setStyleSheet("background-color: #c0392b; color: white; font-weight: bold; padding: 6px;");
    connect(btnDeleteCatalog, &QPushButton::clicked, this, &MainWindow::deleteCatalogItem);

    lblCompareResult = new QLabel("Добавьте товары для сравнения цен.");
    lblCompareResult->setWordWrap(true);
    lblCompareResult->setStyleSheet("font-weight: bold; color: #00a8ff; padding: 10px; background: #e1e2e6; border-radius: 5px;");

    rightLayout->addWidget(tableCatalog);
    rightLayout->addWidget(btnDeleteCatalog);
    rightLayout->addWidget(lblCompareResult);

    mainLayout->addWidget(boxInput, 1);
    mainLayout->addLayout(rightLayout, 2);

    loadCatalogFromFile();

    return tab;
}

void MainWindow::addCatalogItem() {
    if (editProdName->text().isEmpty() || sbProdPrice->value() <= 0) {
        QMessageBox::warning(this, "Ошибка", "Заполните название и цену товара!");
        return;
    }

    CatalogItem item;
    // Приводим к нижнему регистру, чтобы "молоко" и "Молоко" считались одним товаром
    item.productName = editProdName->text().trimmed().toLower();
    item.storeName = cbStoreName->currentText();
    item.price = sbProdPrice->value();

    catalogItems.push_back(item);

    // Сбрасываем поля ввода
    editProdName->clear();
    sbProdPrice->setValue(0);

    // Обновляем UI и сохраняем файл
    comparePrices();
    saveCatalogToFile();
}

void MainWindow::comparePrices() {
    tableCatalog->setRowCount(static_cast<int>(catalogItems.size()));

    // Карта для поиска минимальной цены по каждому уникальному товару
    std::map<QString, double> minPrices;
    for (const auto& item : catalogItems) {
        if (minPrices.find(item.productName) == minPrices.end() || item.price < minPrices[item.productName]) {
            minPrices[item.productName] = item.price;
        }
    }

    // Заполняем таблицу
    for (size_t i = 0; i < catalogItems.size(); ++i) {
        const auto& item = catalogItems[i];

        tableCatalog->setItem(i, 0, new QTableWidgetItem(item.productName));
        tableCatalog->setItem(i, 1, new QTableWidgetItem(item.storeName));

        QTableWidgetItem* priceItem = new QTableWidgetItem(QString::number(item.price, 'f', 2) + " ₽");

        if (item.price == minPrices[item.productName]) {
            priceItem->setForeground(QBrush(QColor("#27ae60"))); // Зеленый цвет текста
            priceItem->setFont(QFont("Arial", -1, QFont::Bold));
        }
        tableCatalog->setItem(i, 2, priceItem);
    }

    // Генерируем краткий умный совет под таблицей
    if (!catalogItems.empty()) {
        QString lastProduct = catalogItems.back().productName;
        QString bestStore = "";
        double minPrice = minPrices[lastProduct];

        for (const auto& item : catalogItems) {
            if (item.productName == lastProduct && item.price == minPrice) {
                bestStore = item.storeName;
                break;
            }
        }
        lblCompareResult->setText(QString("💡 Анализ цен: Товар '<b>%1</b>' выгоднее всего покупать в магазине <b>%2</b> по цене <b>%3 ₽</b>.")
                                      .arg(lastProduct).arg(bestStore).arg(minPrice, 0, 'f', 2));
    }
}

void MainWindow::saveCatalogToFile() {
    QFile file("prices.txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const auto& item : catalogItems) {
            out << item.productName << ";" << item.storeName << ";" << item.price << "\n";
        }
        file.close();
    }
}

void MainWindow::loadCatalogFromFile() {
    QFile file("prices.txt");
    if (!file.exists()) return;

    catalogItems.clear();
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(";");
            if (parts.size() == 3) {
                CatalogItem item;
                item.productName = parts[0];
                item.storeName = parts[1];
                item.price = parts[2].toDouble();
                catalogItems.push_back(item);
            }
        }
        file.close();
        comparePrices();
    }
}

void MainWindow::deleteTransaction() {
    int selectedRow = tableHistory->currentRow();

    if (selectedRow < 0) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, выберите транзакцию в таблице для удаления!");
        return;
    }

    transactions.erase(transactions.begin() + selectedRow);

    saveToFile();
    refreshViews();
    QMessageBox::information(this, "Успех", "Транзакция успешно удалена!");
}

void MainWindow::deleteCatalogItem() {
    int selectedRow = tableCatalog->currentRow();

    if (selectedRow < 0) {
        QMessageBox::warning(this, "Внимание", "Пожалуйста, выберите товар в таблице для удаления!");
        return;
    }

    catalogItems.erase(catalogItems.begin() + selectedRow);
    comparePrices();
    saveCatalogToFile();

    QMessageBox::information(this, "Успех", "Товар удален из каталога!");
}