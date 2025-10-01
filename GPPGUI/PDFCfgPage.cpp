#include "pdfCfgPage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QDesktopServices>

#include "ElaScrollPageArea.h"
#include "ElaToggleSwitch.h"
#include "ElaText.h"
#include "ElaLineEdit.h"
#include "ElaPushButton.h"

import Tool;

PDFCfgPage::PDFCfgPage(toml::ordered_value& projectConfig, QWidget* parent) : BasePage(parent), _projectConfig(projectConfig)
{
	setWindowTitle(tr("PDF 输出配置"));
	setContentsMargins(10, 0, 10, 0);

	// 创建一个中心部件和布局
	QWidget* centerWidget = new QWidget(this);
	QVBoxLayout* mainLayout = new QVBoxLayout(centerWidget);

	// 输出双语翻译文件
	bool outputDual = toml::get_or(_projectConfig["plugins"]["PDF"]["输出双语翻译文件"], true);
	ElaScrollPageArea* outputArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* outputLayout = new QHBoxLayout(outputArea);
	ElaText* outputText = new ElaText(tr("输出双语翻译文件"), outputArea);
	outputText->setTextPixelSize(16);
	outputLayout->addWidget(outputText);
	outputLayout->addStretch();
	ElaToggleSwitch* outputSwitch = new ElaToggleSwitch(outputArea);
	outputSwitch->setIsToggled(outputDual);
	outputLayout->addWidget(outputSwitch);
	mainLayout->addWidget(outputArea);

	// PDF转换器路径
	std::string pdfConverterPath = toml::get_or(_projectConfig["plugins"]["PDF"]["PDFConverterPath"], "BaseConfig/PDFConverter/PDFConverter.exe");
	ElaScrollPageArea* converterArea = new ElaScrollPageArea(centerWidget);
	QHBoxLayout* converterLayout = new QHBoxLayout(converterArea);
	ElaText* converterText = new ElaText(tr("PDF转换器路径"), converterArea);
	converterText->setTextPixelSize(16);
	converterLayout->addWidget(converterText);
	converterLayout->addStretch();
	ElaLineEdit* converterEdit = new ElaLineEdit(converterArea);
	converterEdit->setText(QString::fromStdString(pdfConverterPath));
	converterEdit->setFixedWidth(600);
	converterLayout->addWidget(converterEdit);
	ElaPushButton* converterButton = new ElaPushButton(tr("浏览"), converterArea);
	converterLayout->addWidget(converterButton);
	connect(converterButton, &ElaPushButton::clicked, this, [=]()
		{
			// 打开文件选择对话框
			QString filePath = QFileDialog::getOpenFileName(this, tr("选择PDF转换器"), "BaseConfig/PDFConverter", "executable files (*.exe *.bat *.sh *.py);; All files (*.*)");
			if (!filePath.isEmpty()) {
				converterEdit->setText(filePath);
			}
		});
	mainLayout->addWidget(converterArea);

	QWidget* tipButtonWidget = new QWidget(centerWidget);
	QHBoxLayout* tipLayout = new QHBoxLayout(tipButtonWidget);
	tipLayout->addStretch();
	ElaPushButton* tipButton = new ElaPushButton(tr("说明"), tipButtonWidget);
	tipLayout->addWidget(tipButton);
	connect(tipButton, &ElaPushButton::clicked, this, [=]()
		{
			QDesktopServices::openUrl(QUrl::fromLocalFile("BaseConfig/illustration/pdf.html"));
		});
	mainLayout->addWidget(tipButtonWidget);

	_applyFunc = [=]()
		{
			insertToml(_projectConfig, "plugins.PDF.输出双语翻译文件", outputSwitch->getIsToggled());
			insertToml(_projectConfig, "plugins.PDF.PDFConverterPath", converterEdit->text().toStdString());
		};

	mainLayout->addStretch();
	centerWidget->setWindowTitle(tr("PDF 输出配置"));
	addCentralWidget(centerWidget, true, true, 0);
}

PDFCfgPage::~PDFCfgPage()
{

}
