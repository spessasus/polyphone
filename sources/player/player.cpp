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

#include "player.h"
#include "ui_player.h"
#include "contextmanager.h"
#include "soundfontmanager.h"
#include "playertreeproxymodel.h"
#include "playeroptions.h"

Player::Player(PlayerOptions * playerOptions, QWidget * parent) : Tab(parent),
    ui(new Ui::Player),
    _playerOptions(new PlayerOptions(playerOptions)),
    _initializing(true),
    _synth(ContextManager::audio()->getSynth())
{
    memset(_currentKeyVelocities, 0, 128 * sizeof(int));
    ui->setupUi(this);

    QString highlightColorBackground = ContextManager::theme()->getColor(ThemeManager::HIGHLIGHTED_BACKGROUND).name();
    QString highlightColorText = ContextManager::theme()->getColor(ThemeManager::HIGHLIGHTED_TEXT).name();
    ui->topBar->setStyleSheet("QWidget#topBar{background-color:" + highlightColorBackground + "} QLabel{color:" + highlightColorText + "}");

    // Keyboard with all keys (128)
    ui->keyboard->set(PianoKeybd::PROPERTY_KEY_MIN, 0);
    ui->keyboard->set(PianoKeybd::PROPERTY_KEY_NUMBER, 128);

    // Initialize the configuration
    ui->comboChannel->setCurrentIndex(_playerOptions->playerChannel());
    ui->comboMultipleSelection->setCurrentIndex(_playerOptions->playerMultipleSelection() ? 1 : 0);
    ui->comboSelectionByKeys->setCurrentIndex(_playerOptions->playerKeySelection());
    _initializing = false;

    on_comboChannel_currentIndexChanged(ui->comboChannel->currentIndex());
    on_comboMultipleSelection_currentIndexChanged(ui->comboMultipleSelection->currentIndex());
}

Player::~Player()
{
    delete ui;
    delete _playerOptions;
}

void Player::tabInitializing(QString filename)
{
    Q_UNUSED(filename);
    ui->rotatingSpinner->startAnimation();
}

void Player::tabInError(QString errorMessage)
{
    // Display the error
    ui->labelReason->setText(errorMessage);
    ui->stackedMain->setCurrentWidget(ui->pageError);
}

void Player::tabInitialized(int indexSf2)
{
    _currentSoundfontId = indexSf2;

    // Prepare the lists
    QAbstractItemModel * model = SoundfontManager::getInstance()->getModel(indexSf2);
    PlayerTreeProxyModel * proxyModel = new PlayerTreeProxyModel(indexSf2, model);
    ui->listBank->setModel(proxyModel);
    ui->listPreset->setModel(proxyModel);
    connect(ui->listBank->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onBankSelectionChanged(QItemSelection,QItemSelection)));
    connect(ui->listPreset->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(onPresetSelectionChanged(QItemSelection,QItemSelection)));

    // Display the player controls
    ui->stackedMain->setCurrentWidget(ui->pagePlay);

    // Select the first bank
    ui->listBank->setCurrentIndex(ui->listBank->model()->index(0, 0, QModelIndex()));
}

void Player::tabUpdate(QString editingSource)
{
    // No editing in this mode
    Q_UNUSED(editingSource)
}

void Player::onBankSelectionChanged(QItemSelection selected, QItemSelection deselected)
{
    Q_UNUSED(deselected)
    QModelIndexList indexes = selected.indexes();
    if (indexes.count() != 1)
        return;

    // Mute everything
    _currentIds.clear();
    memset(_currentKeyVelocities, 0, 128 * sizeof(int));
    _synth->stop(true);

    // Populate the preset list
    ui->listPreset->setRootIndex(indexes[0]);

    // Select the first preset
    ui->listPreset->selectionModel()->clear();
    ui->listPreset->setCurrentIndex(ui->listPreset->model()->index(0, 0, indexes[0]));

    // Scan the presets within this bank
    for (int i = 0; i < 128; i++)
        _presetPositionByPresetNumber[i] = -1;
    for (int i = 0; i < ui->listPreset->model()->rowCount(indexes[0]); i++)
    {
        QModelIndex presetIndex = ui->listPreset->model()->index(i, 0, indexes[0]);
        _presetPositionByPresetNumber[presetIndex.data(Qt::UserRole + 1).toInt()] = i;
    }
}

void Player::onPresetSelectionChanged(QItemSelection selected, QItemSelection deselected)
{
    // Remove deselected ids
    QModelIndexList indexes = deselected.indexes();
    foreach (QModelIndex index, indexes)
    {
        EltID id = index.data(Qt::UserRole).value<EltID>();
        _currentIds.removeAll(id);

        // Mute deselected presets
        for (int key = 0; key < 128; key++)
            if (_currentKeyVelocities[key] > 0)
                _synth->play(id, _playerOptions->playerChannel(), key, 0);
    }

    // Add selected ids
    indexes = selected.indexes();
    foreach (QModelIndex index, indexes)
    {
        EltID id = index.data(Qt::UserRole).value<EltID>();
        _currentIds.append(id);

        // Play selected presets using tthe same velocity
        for (int key = 0; key < 128; key++)
            if (_currentKeyVelocities[key] > 0)
                _synth->play(id, _playerOptions->playerChannel(), key, _currentKeyVelocities[key]);
    }

    // Possible keys on the keyboard
    this->customizeKeyboard();
}

bool Player::processKey(int channel, int key, int vel)
{
    if (channel != _playerOptions->playerChannel() - 1 && _playerOptions->playerChannel() != 0)
        return false;

    if (_playerOptions->playerKeySelection() != 0 && _presetPositionByPresetNumber[key] != -1)
    {
        QItemSelectionModel::SelectionFlags selectionFlags;
        if (_playerOptions->playerKeySelection() == 1)
            selectionFlags = vel == 0 ? QItemSelectionModel::Deselect : QItemSelectionModel::Select;
        else if (vel > 0)
            selectionFlags = QItemSelectionModel::Toggle;
        else
            return false;

        if (!_playerOptions->playerMultipleSelection())
            selectionFlags |= QItemSelectionModel::Clear;

        ui->listPreset->selectionModel()->select(
            ui->listPreset->model()->index(_presetPositionByPresetNumber[key], 0, ui->listPreset->rootIndex()),
            selectionFlags);
        return false;
    }

    // Update the current velocities
    _currentKeyVelocities[key] = vel;

    foreach (EltID presetId, _currentIds)
        _synth->play(presetId, channel, key, vel);

    if (vel > 0)
        ui->keyboard->inputNoteOn(key);
    else
        ui->keyboard->inputNoteOff(key);

    return false;
}

void Player::on_comboChannel_currentIndexChanged(int index)
{
    if (_initializing)
        return;

    // Mute the current channel
    for (int key = 0; key < 128; key++)
    {
        if (_currentKeyVelocities[key] > 0)
        {
            _currentKeyVelocities[key] = 0;
            foreach (EltID presetId, _currentIds)
                _synth->play(presetId, _playerOptions->playerChannel(), key, 0);
        }
    }

    _playerOptions->setPlayerChannel(index);
    ui->keyboard->set(PianoKeybd::PROPERTY_CHANNEL, index - 1);
    ui->controllerArea->setChannel(index - 1);
}

void Player::on_comboMultipleSelection_currentIndexChanged(int index)
{
    if (_initializing)
        return;
    _playerOptions->setMultipleSelection(index == 1);
    ui->listPreset->setSelectionMode(index == 1 ? QAbstractItemView::MultiSelection : QAbstractItemView::SingleSelection);

    if (index == 0)
    {
        // Keep only the last selected element
        QItemSelection selection = ui->listPreset->selectionModel()->selection();
        selection.removeLast();
        if (!selection.empty())
            ui->listPreset->selectionModel()->select(selection, QItemSelectionModel::Deselect);
    }
}

void Player::on_comboSelectionByKeys_currentIndexChanged(int index)
{
    if (_initializing)
        return;
    _playerOptions->setKeySelection(index);

    if (index == 1)
    {
        // Deselect presets if the corresponding key is not on
        QModelIndexList indexes = ui->listPreset->selectionModel()->selection().indexes();
        foreach (QModelIndex index, indexes)
        {
            int selectedPresetNumber = index.data(Qt::UserRole + 1).toInt();
            if (_currentKeyVelocities[selectedPresetNumber] == 0)
            {
                EltID id = index.data(Qt::UserRole).value<EltID>();
                for (int key = 0; key < 128; key++)
                    if (_currentKeyVelocities[key] > 0)
                        _synth->play(id, _playerOptions->playerChannel(), key, 0);
            }
        }
    }
}

void Player::customizeKeyboard()
{
    // Scan all presets
    bool enabledKeys[128];
    memset(enabledKeys, 0, 128 * sizeof(bool));
    foreach (EltID id, _currentIds)
        scanPreset(id, enabledKeys);

    // Clear everything
    ui->keyboard->clearCustomization();

    // Create ranges of keys that can be played
    int startKey = -1;
    for (int key = 0; key < 128; key++)
    {
        if (enabledKeys[key])
        {
            // Possibly start a range
            if (startKey == -1)
                startKey = key;
        }
        else
        {
            if (startKey != -1)
            {
                // Close a range
                ui->keyboard->addRangeAndRootKey(-1, startKey, key - 1);
                startKey = -1;
            }
        }
    }

    // Final range
    if (startKey != -1)
        ui->keyboard->addRangeAndRootKey(-1, startKey, 127);
}

void Player::scanPreset(EltID presetId, bool * keys)
{
    SoundfontManager * sm = SoundfontManager::getInstance();

    // Default key range of the preset
    RangesType defaultKeyRange;
    defaultKeyRange.byLo = 0;
    defaultKeyRange.byHi = 127;
    if (sm->isSet(presetId, champ_keyRange))
        defaultKeyRange = sm->get(presetId, champ_keyRange).rValue;

    // Scan preset divisions
    EltID prstInstId(elementPrstInst, presetId.indexSf2, presetId.indexElt);
    foreach (int i, sm->getSiblings(prstInstId))
    {
        prstInstId.indexElt2 = i;

        // Maximum key range of this division
        RangesType maxRange = defaultKeyRange;
        if (sm->isSet(prstInstId, champ_keyRange))
            maxRange = sm->get(prstInstId, champ_keyRange).rValue;

        // Instrument target by this division
        int instId = sm->get(prstInstId, champ_instrument).wValue;
        if (!_rangeByInst.contains(instId))
            _rangeByInst[instId] = getEnabledKeysForInstrument(EltID(elementInst, prstInstId.indexSf2, instId));

        for (int key = 0; key < 128; key++)
            keys[key] |= _rangeByInst[instId][key];
    }
}

QVector<bool> Player::getEnabledKeysForInstrument(EltID idInst)
{
    SoundfontManager * sm = SoundfontManager::getInstance();
    QVector<bool> result(128);
    result.fill(false);

    // Default range of this instrument
    RangesType defInstRange;
    if (sm->isSet(idInst, champ_keyRange))
        defInstRange = sm->get(idInst, champ_keyRange).rValue;
    else
    {
        defInstRange.byLo = 0;
        defInstRange.byHi = 127;
    }

    // Browse all divisions inside the instrument
    EltID idInstSmpl(elementInstSmpl, idInst.indexSf2, idInst.indexElt);
    foreach (int j, sm->getSiblings(idInstSmpl))
    {
        idInstSmpl.indexElt2 = j;
        RangesType instDivRange = defInstRange;
        if (sm->isSet(idInstSmpl, champ_keyRange))
            instDivRange = sm->get(idInstSmpl, champ_keyRange).rValue;

        // And set all used keys to "true"
        for (unsigned int key = instDivRange.byLo; key <= instDivRange.byHi; key++)
            result[key] = true;
    }

    return result;
}
