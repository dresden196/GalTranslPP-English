#include "pdfCfgPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>

#include "ElaScrollPageArea.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"

import Tool;

PDFCfgPage::PDFCfgPage(toml::table& projectConfig, QWidget* parent) : BasePage(parent), _projectConfig(projectConfig)
{
	setWindowTitle("PDF 输出配置");
	setContentsMargins(10, 0, 10, 0);

	// 创建一个中心部件和布局
	QWidget* centerWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(centerWidget);

	// 输出双语翻译文件
	bool outputDual = _projectConfig["plugins"]["PDF"]["输出双语翻译文件"].value_or(true);
	ElaScrollPageArea* outputArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* outputLayout = new QHBoxLayout(outputArea);
	ElaText* outputText = new ElaText("输出双语翻译文件", outputArea);
	outputText->setTextPixelSize(16);
	outputLayout->addWidget(outputText);
	outputLayout->addStretch();
	ElaToggleSwitch* outputSwitch = new ElaToggleSwitch(outputArea);
	outputSwitch->setIsToggled(outputDual);
	outputLayout->addWidget(outputSwitch);
	mainLayout->addWidget(outputArea);

	// PDF转换器路径
	std::string pdfConverterPath = _projectConfig["plugins"]["PDF"]["PDFConverterPath"].value_or("BaseConfig/PDFConverter/PDFConverter.exe");
	ElaScrollPageArea* converterArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* converterLayout = new QHBoxLayout(converterArea);
	ElaText* converterText = new ElaText("PDF转换器路径", converterArea);
	converterText->setTextPixelSize(16);
	converterLayout->addWidget(converterText);
	converterLayout->addStretch();
	ElaLineEdit* converterEdit = new ElaLineEdit(converterArea);
	converterEdit->setText(QString::fromStdString(pdfConverterPath));
	converterEdit->setFixedWidth(600);
	converterLayout->addWidget(converterEdit);
	ElaPushButton* converterButton = new ElaPushButton("浏览", converterArea);
	converterLayout->addWidget(converterButton);
	connect(converterButton, &ElaPushButton::clicked, this, [=]()
		{
			// 打开文件选择对话框
			QString filePath = QFileDialog::getOpenFileName(this, "选择PDF转换器", "BaseConfig/PDFConverter", "可执行文件或脚本 (*.exe *.bat *.sh *.py)");
			if (!filePath.isEmpty()) {
				converterEdit->setText(filePath);
			}
		});
	mainLayout->addWidget(converterArea);

	_applyFunc = [=]()
		{
			insertToml(_projectConfig, "plugins.PDF.输出双语翻译文件", outputSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.PDF.PDFConverterPath", converterEdit->text().toStdString());
		};

	mainLayout->addStretch();
	centerWidget->setWindowTitle("PDF 输出配置");
	addCentralWidget(centerWidget, true, true, 0);
}

PDFCfgPage::~PDFCfgPage()
{

}
