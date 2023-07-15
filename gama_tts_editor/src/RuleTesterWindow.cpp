/***************************************************************************
 *  Copyright 2014, 2015 Marcelo Y. Matuda                                 *
 *  Copyright 1991, 1992, 1993, 1994, 1995, 1996, 2001, 2002               *
 *    David R. Hill, Leonard Manzara, Craig Schock                         *
 *                                                                         *
 *  This program is free software: you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation, either version 3 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *  You should have received a copy of the GNU General Public License      *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/
// 2015-01
// This file was created by Marcelo Y. Matuda, and code/information
// from Gnuspeech was added to it later.

#include "RuleTesterWindow.h"

#include <QMessageBox>

#include "Model.h"
#include "ui_RuleTesterWindow.h"



namespace GS {

RuleTesterWindow::RuleTesterWindow(QWidget* parent)
		: QWidget(parent)
		, ui_(std::make_unique<Ui::RuleTesterWindow>())
		, model_()
{
	ui_->setupUi(this);
}

RuleTesterWindow::~RuleTesterWindow()
{
}

void
RuleTesterWindow::resetModel(VTMControlModel::Model* model)
{
	model_ = model;
}

void
RuleTesterWindow::on_shiftButton_clicked()
{
	ui_->posture1LineEdit->setText(ui_->posture2LineEdit->text());
	ui_->posture2LineEdit->setText(ui_->posture3LineEdit->text());
	ui_->posture3LineEdit->setText(ui_->posture4LineEdit->text());
	ui_->posture4LineEdit->clear();
}

void
RuleTesterWindow::on_testButton_clicked()
{
	if (model_ == nullptr) return;

	std::vector<VTMControlModel::RuleExpressionData> ruleExpressionData;

	QString posture1Text = ui_->posture1LineEdit->text().trimmed();
	if (posture1Text.isEmpty()) {
		clearResults();
		return;
	}
	bool post1Marked = false;
	if (posture1Text.endsWith('\'')) {
		posture1Text.remove(posture1Text.size() - 1, 1);
		post1Marked = true;
	}
	const VTMControlModel::Posture* posture1 = model_->postureList().find(posture1Text.toStdString());
	if (posture1 == nullptr) {
		clearResults();
		QMessageBox::warning(this, tr("Warning"), tr("Posture 1 not found."));
		return;
	} else {
		ruleExpressionData.push_back(VTMControlModel::RuleExpressionData{posture1, 1.0, post1Marked});
	}

	QString posture2Text = ui_->posture2LineEdit->text().trimmed();
	if (posture2Text.isEmpty()) {
		clearResults();
		return;
	}
	bool post2Marked = false;
	if (posture2Text.endsWith('\'')) {
		posture2Text.remove(posture2Text.size() - 1, 1);
		post2Marked = true;
	}
	const VTMControlModel::Posture* posture2 = model_->postureList().find(posture2Text.toStdString());
	if (posture2 == nullptr) {
		clearResults();
		QMessageBox::warning(this, tr("Warning"), tr("Posture 2 not found."));
		return;
	} else {
		ruleExpressionData.push_back(VTMControlModel::RuleExpressionData{posture2, 1.0, post2Marked});
	}

	QString posture3Text = ui_->posture3LineEdit->text().trimmed();
	if (!posture3Text.isEmpty()) {
		bool post3Marked = false;
		if (posture3Text.endsWith('\'')) {
			posture3Text.remove(posture3Text.size() - 1, 1);
			post3Marked = true;
		}
		const VTMControlModel::Posture* posture3 = model_->postureList().find(posture3Text.toStdString());
		if (posture3 == nullptr) {
			clearResults();
			QMessageBox::warning(this, tr("Warning"), tr("Posture 3 not found."));
			return;
		} else {
			ruleExpressionData.push_back(VTMControlModel::RuleExpressionData{posture3, 1.0, post3Marked});
		}

		QString posture4Text = ui_->posture4LineEdit->text().trimmed();
		if (!posture4Text.isEmpty()) {
			bool post4Marked = false;
			if (posture4Text.endsWith('\'')) {
				posture4Text.remove(posture4Text.size() - 1, 1);
				post4Marked = true;
			}
			const VTMControlModel::Posture* posture4 = model_->postureList().find(posture4Text.toStdString());
			if (posture4 == nullptr) {
				clearResults();
				QMessageBox::warning(this, tr("Warning"), tr("Posture 4 not found."));
				return;
			} else {
				ruleExpressionData.push_back(VTMControlModel::RuleExpressionData{posture4, 1.0, post4Marked});
			}
		}
	}

	unsigned int ruleIndex;
	const VTMControlModel::Rule* rule = model_->findFirstMatchingRule(ruleExpressionData, ruleIndex);
	if (rule == nullptr) {
		clearResults();
		QMessageBox::critical(this, tr("Error"), tr("Could not find a matching rule."));
		return;
	}

	QString ruleText = QString("%1. ").arg(ruleIndex + 1U);
	for (unsigned int i = 0, size = rule->booleanExpressionList().size(); i < size; ++i) {
		if (i > 0) {
			ruleText += " >> ";
		}
		ruleText += rule->booleanExpressionList()[i].c_str();
	}
	ui_->ruleLineEdit->setText(ruleText);

	ui_->consumedTokensLineEdit->setText(QString::number(rule->numberOfExpressions()));

	double ruleSymbols[VTMControlModel::Rule::NUM_SYMBOLS];
	rule->evaluateExpressionSymbols(ruleExpressionData, *model_, ruleSymbols);

	ui_->durationLineEdit->setText(QString::number(ruleSymbols[VTMControlModel::Rule::SYMB_DURATION]));
	ui_->beatLineEdit->setText(    QString::number(ruleSymbols[VTMControlModel::Rule::SYMB_BEAT]));
	ui_->mark1LineEdit->setText(   QString::number(ruleSymbols[VTMControlModel::Rule::SYMB_MARK1]));
	ui_->mark2LineEdit->setText(   QString::number(ruleSymbols[VTMControlModel::Rule::SYMB_MARK2]));
	ui_->mark3LineEdit->setText(   QString::number(ruleSymbols[VTMControlModel::Rule::SYMB_MARK3]));
}

void
RuleTesterWindow::clearResults()
{
	ui_->ruleLineEdit->clear();
	ui_->consumedTokensLineEdit->clear();
	ui_->durationLineEdit->clear();
	ui_->beatLineEdit->clear();
	ui_->mark1LineEdit->clear();
	ui_->mark2LineEdit->clear();
	ui_->mark3LineEdit->clear();
}

} // namespace GS
