#include "CommandPage.hpp"

#include "Application.hpp"
#include "controllers/commands/Command.hpp"
#include "controllers/commands/CommandController.hpp"
#include "controllers/commands/CommandModel.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/LayoutCreator.hpp"
#include "util/Qt.hpp"
#include "util/StandardItemHelper.hpp"
#include "widgets/helper/EditableModelView.hpp"
#include "widgets/TooltipWidget.hpp"

#include <QColor>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTableView>
#include <QTextEdit>

// clang-format off
#define TEXT "{1} => first word &nbsp;&nbsp;&nbsp; {1+} => first word and after &nbsp;&nbsp;&nbsp; {{ => { &nbsp;&nbsp;&nbsp; <a href='https://chatterino.com/help/commands'>more info</a>"
// clang-format on

namespace chatterino {
namespace {
    QString c1settingsPath()
    {
        return combinePath(qgetenv("appdata"),
                           "Chatterino\\Custom\\Commands.txt");
    }
}  // namespace

CommandPage::CommandPage()
{
    auto app = getApp();

    LayoutCreator<CommandPage> layoutCreator(this);
    auto layout = layoutCreator.setLayoutType<QVBoxLayout>();

    EditableModelView *view =
        layout.emplace<EditableModelView>(app->commands->createModel(nullptr))
            .getElement();

    view->setTitles({"Trigger", "Command", "Show In\nMessage Menu"});
    view->getTableView()->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    view->addButtonPressed.connect([] {
        getApp()->commands->items.append(
            Command{"/command", "I made a new command HeyGuys"});
    });

    QItemSelectionModel *selectionModel =
        view->getTableView()->selectionModel();
    QObject::connect(
        selectionModel, &QItemSelectionModel::currentChanged, this,
        [this, view](const QModelIndex &current, const QModelIndex &previous) {
            if (this->handleCommandDuplicates(view))
            {
                this->duplicateCommandWarning->show();
            }
            else
            {
                this->duplicateCommandWarning->hide();
            }
        });

    bool duplicatesExist = this->handleCommandDuplicates(view);

    // TODO: asyncronously check path
    if (QFile(c1settingsPath()).exists())
    {
        auto button = new QPushButton("Import commands from Chatterino 1");
        view->addCustomButton(button);

        QObject::connect(button, &QPushButton::clicked, this, [] {
            QFile c1settings = c1settingsPath();
            c1settings.open(QIODevice::ReadOnly);
            for (auto line :
                 QString(c1settings.readAll())
                     .split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts))
            {
                if (int index = line.indexOf(' '); index != -1)
                {
                    getApp()->commands->items.insert(
                        Command(line.mid(0, index), line.mid(index + 1)));
                }
            }
        });
    }

    layout.append(
        this->createCheckBox("Also match the trigger at the end of the message",
                             getSettings()->allowCommandsAtEnd));

    QLabel *text = layout.emplace<QLabel>(TEXT).getElement();
    text->setWordWrap(true);
    text->setStyleSheet("color: #bbb");
    text->setOpenExternalLinks(true);

    this->duplicateCommandWarning =
        layout.emplace<QLabel>("Duplicate trigger names detected").getElement();
    this->duplicateCommandWarning->setStyleSheet("color: yellow");
    if (duplicatesExist)
    {
        this->duplicateCommandWarning->show();
    }
    else
    {
        this->duplicateCommandWarning->hide();
    }

    // ---- end of layout
    this->commandsEditTimer_.setSingleShot(true);
}

bool CommandPage::handleCommandDuplicates(EditableModelView *view)
{
    bool retval = false;
    QMap<QString, QList<int>> map = *new QMap<QString, QList<int>>();
    for (int i = 0; i < view->getModel()->rowCount(); i++)
    {
        QString commandName = view->getModel()->index(i, 0).data().toString();
        if (map.contains(commandName))
        {
            QList<int> value = map[commandName];
            value.append(i);
            map.insert(commandName, value);
        }
        else
        {
            map.insert(commandName, {i});
        }
    }

    foreach (const QString &key, map.keys())
    {
        if (map[key].length() != 1)
        {
            retval = true;
            foreach (int value, map[key])
            {
                view->getModel()->setData(view->getModel()->index(value, 0),
                                          QColor("yellow"), Qt::ForegroundRole);
            }
        }
        else
        {
            view->getModel()->setData(view->getModel()->index(map[key][0], 0),
                                      QColor("white"), Qt::ForegroundRole);
        }
    }

    return retval;
}

}  // namespace chatterino
