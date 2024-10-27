/***************************************************************************
**                                                                        **
**  Polyphone, a soundfont editor                                         **
**  Copyright (C) 2013-2024 Davy Triponney                                **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program. If not, see http://www.gnu.org/licenses/.    **
**                                                                        **
****************************************************************************
**           Author: Davy Triponney                                       **
**  Website/Contact: https://www.polyphone.io                             **
**             Date: 01.01.2013                                           **
***************************************************************************/

#include "toolglobalsettings_gui.h"
#include "ui_toolglobalsettings_gui.h"
#include "toolglobalsettings_parameters.h"
#include "soundfontmanager.h"
#include "graphparamglobal.h"
#include "utils.h"

ToolGlobalSettings_gui::ToolGlobalSettings_gui(QWidget *parent) :
    AbstractToolGui(parent),
    ui(new Ui::ToolGlobalSettings_gui)
{
    ui->setupUi(this);
}

ToolGlobalSettings_gui::~ToolGlobalSettings_gui()
{
    delete ui;
}

void ToolGlobalSettings_gui::updateInterface(AbstractToolParameters * parameters, IdList ids)
{
    _isInst = (ids.isEmpty() || ids[0].typeElement == elementInst || ids[0].typeElement == elementInstSmpl);
    ToolGlobalSettings_parameters * params = dynamic_cast<ToolGlobalSettings_parameters *>(parameters);

    // Attribute list
    QList<AttributeType> paramTypeList;
    paramTypeList << champ_initialAttenuation
                  << champ_pan;
    if (_isInst)
        paramTypeList << champ_overridingRootKey;
    paramTypeList << champ_keyRange
                  << champ_velRange
                  << champ_coarseTune
                  << champ_fineTune
                  << champ_scaleTuning
                  << champ_initialFilterFc
                  << champ_initialFilterQ
                  << champ_delayVolEnv
                  << champ_attackVolEnv
                  << champ_holdVolEnv
                  << champ_decayVolEnv
                  << champ_sustainVolEnv
                  << champ_releaseVolEnv
                  << champ_keynumToVolEnvHold
                  << champ_keynumToVolEnvDecay
                  << champ_delayModEnv
                  << champ_attackModEnv
                  << champ_holdModEnv
                  << champ_decayModEnv
                  << champ_sustainModEnv
                  << champ_releaseModEnv
                  << champ_modEnvToPitch
                  << champ_modEnvToFilterFc
                  << champ_keynumToModEnvHold
                  << champ_keynumToModEnvDecay
                  << champ_delayModLFO
                  << champ_freqModLFO
                  << champ_modLfoToPitch
                  << champ_modLfoToFilterFc
                  << champ_modLfoToVolume
                  << champ_delayVibLFO
                  << champ_freqVibLFO
                  << champ_vibLfoToPitch
                  << champ_chorusEffectsSend
                  << champ_reverbEffectsSend;

    // Last attribute
    ui->comboAttribute->blockSignals(true);
    for (int i = 0; i < paramTypeList.size(); i++)
    {
        ui->comboAttribute->addItem(Attribute::getDescription(paramTypeList.at(i), !_isInst));
        ui->comboAttribute->setItemData(i, static_cast<int>(paramTypeList.at(i)));
    }
    AttributeType attributeToSelect = static_cast<AttributeType>(_isInst ? params->getInstAttribute() : params->getPrstAttribute());
    ui->comboAttribute->setCurrentIndex(paramTypeList.indexOf(attributeToSelect));
    if (ui->comboAttribute->currentIndex() == -1)
        ui->comboAttribute->setCurrentIndex(paramTypeList.indexOf(champ_initialAttenuation));
    ui->comboAttribute->blockSignals(false);

    // Pattern type
    ui->comboPattern->blockSignals(true);
    ui->comboPattern->setCurrentIndex(_isInst ? params->getInstPattern() : params->getPrstPattern());
    ui->comboPattern->blockSignals(false);

    // Pattern parameter
    ui->doubleSpinParam->blockSignals(true);
    ui->doubleSpinParam->setValue(_isInst ? params->getInstParam() : params->getPrstParam());
    ui->doubleSpinParam->blockSignals(false);

    // Min / max
    ui->doubleSpinMin->blockSignals(true);
    ui->doubleSpinMin->setValue(_isInst ? params->getInstMin() : params->getPrstMin());
    ui->doubleSpinMin->blockSignals(false);

    ui->doubleSpinMax->blockSignals(true);
    ui->doubleSpinMax->setValue(_isInst ? params->getInstMax() : params->getPrstMax());
    ui->doubleSpinMax->blockSignals(false);

    ui->graphParamGlobal->setMinMax(ui->doubleSpinMin->value(), ui->doubleSpinMax->value());

    // Modification type
    ui->comboModif->blockSignals(true);
    ui->comboModif->setCurrentIndex(_isInst ? params->getInstModifType() : params->getPrstModifType());
    ui->comboModif->blockSignals(false);

    // Velocity range
    ui->spinVelMin->setValue(_isInst ? params->getInstMinVel() : params->getPrstMinVel());
    ui->spinVelMax->setValue(_isInst ? params->getInstMaxVel() : params->getPrstMaxVel());

    // Draw graph
    this->on_comboPattern_currentIndexChanged(ui->comboPattern->currentIndex());
    ui->graphParamGlobal->setMinMaxX(Utils::round32(_isInst ? params->getInstMinX() : params->getPrstMinX()),
                                     Utils::round32(_isInst ? params->getInstMaxX() : params->getPrstMaxX()));
    ui->graphParamGlobal->setValues(_isInst ? params->getInstValues() : params->getPrstValues());

    // Key range to highlight
    int minKey = -1;
    int maxKey = -1;
    IdList processedElts;
    SoundfontManager * sm = SoundfontManager::getInstance();
    foreach (EltID id, ids)
    {
        // Current instrument or preset
        EltID idParent = id;
        idParent.typeElement = (id.typeElement == elementInst || id.typeElement == elementInstSmpl) ?
                    elementInst : elementPrst;
        if (processedElts.contains(idParent))
            continue;
        else
            processedElts << idParent;

        // Default range
        RangesType defRange;
        if (sm->isSet(idParent, champ_keyRange))
            defRange = sm->get(idParent, champ_keyRange).rValue;
        else
        {
            defRange.byLo = 0;
            defRange.byHi = 127;
        }

        // Browse all children
        EltID idChild = idParent;
        idChild.typeElement = (idParent.typeElement == elementInst ? elementInstSmpl : elementPrstInst);
        foreach (int index, sm->getSiblings(idChild))
        {
            idChild.indexElt2 = index;
            if (sm->isSet(idChild, champ_keyRange))
            {
                // Compare with the child range
                RangesType rangeTmp = sm->get(idChild, champ_keyRange).rValue;
                if (minKey == -1 || minKey > rangeTmp.byLo)
                    minKey = rangeTmp.byLo;
                if (maxKey == -1 || maxKey < rangeTmp.byHi)
                    maxKey = rangeTmp.byHi;
            }
            else
            {
                // Compare with the default range
                if (minKey == -1 || minKey > defRange.byLo)
                    minKey = defRange.byLo;
                if (maxKey == -1 || maxKey < defRange.byHi)
                    maxKey = defRange.byHi;
            }
        }
    }
    ui->graphParamGlobal->setHighlightedRange(minKey, maxKey);
}

void ToolGlobalSettings_gui::saveParameters(AbstractToolParameters * parameters)
{
    ToolGlobalSettings_parameters * params = dynamic_cast<ToolGlobalSettings_parameters *>(parameters);

    // Store data
    if (_isInst)
    {
        // Curve description
        params->setInstPattern(ui->comboPattern->currentIndex());
        params->setInstParam(ui->doubleSpinParam->value());
        params->setInstMin(ui->doubleSpinMin->value());
        params->setInstMax(ui->doubleSpinMax->value());
        params->setInstMinX(ui->graphParamGlobal->getXmin());
        params->setInstMaxX(ui->graphParamGlobal->getXmax());

        // Values
        params->setInstValues(ui->graphParamGlobal->getValues());

        // Modification
        params->setInstModifType(ui->comboModif->currentIndex());
        params->setInstAttribute(ui->comboAttribute->itemData(ui->comboAttribute->currentIndex()).toInt());
        params->setInstMinVel(ui->spinVelMin->value());
        params->setInstMaxVel(ui->spinVelMax->value());
    }
    else
    {
        // Curve description
        params->setPrstPattern(ui->comboPattern->currentIndex());
        params->setPrstParam(ui->doubleSpinParam->value());
        params->setPrstMin(ui->doubleSpinMin->value());
        params->setPrstMax(ui->doubleSpinMax->value());
        params->setPrstMinX(ui->graphParamGlobal->getXmin());
        params->setPrstMaxX(ui->graphParamGlobal->getXmax());

        // Values
        params->setPrstValues(ui->graphParamGlobal->getValues());

        // Modification
        params->setPrstModifType(ui->comboModif->currentIndex());
        params->setPrstAttribute(ui->comboAttribute->itemData(ui->comboAttribute->currentIndex()).toInt());
        params->setPrstMinVel(ui->spinVelMin->value());
        params->setPrstMaxVel(ui->spinVelMax->value());
    }
}

void ToolGlobalSettings_gui::on_comboPattern_currentIndexChanged(int index)
{
    if (index == 3 || index == 4)
    {
        ui->label_6->setText(tr("Stiffness"));
        ui->doubleSpinParam->setEnabled(true);
    }
    else if (index == 5)
    {
        ui->label_6->setText(tr("Distribution"));
        ui->doubleSpinParam->setEnabled(true);
    }
    else
    {
        ui->label_6->setText("-");
        ui->doubleSpinParam->setEnabled(false);
    }
    ui->graphParamGlobal->indexMotifChanged(index);
}

void ToolGlobalSettings_gui::on_doubleSpinParam_valueChanged(double arg1)
{
    ui->graphParamGlobal->raideurChanged(arg1);
}

void ToolGlobalSettings_gui::on_doubleSpinMin_valueChanged(double arg1)
{
    ui->graphParamGlobal->setMinMax(arg1, ui->doubleSpinMax->value());
}

void ToolGlobalSettings_gui::on_doubleSpinMax_valueChanged(double arg1)
{
    ui->graphParamGlobal->setMinMax(ui->doubleSpinMin->value(), arg1);
}

void ToolGlobalSettings_gui::on_pushCancel_clicked()
{
    emit this->canceled();
}

void ToolGlobalSettings_gui::on_pushOk_clicked()
{
    emit this->validated();
}
