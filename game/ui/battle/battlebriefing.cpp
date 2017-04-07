#include "game/ui/battle/battlebriefing.h"
#include "forms/form.h"
#include "forms/graphic.h"
#include "forms/graphicbutton.h"
#include "forms/label.h"
#include "forms/ui.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "framework/framework.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/city/building.h"
#include "game/state/gamestate.h"
#include "game/ui/battle/battleprestart.h"
#include "game/ui/battle/battleview.h"
#include "game/ui/city/cityview.h"
#include <cmath>

namespace OpenApoc
{

namespace
{
static const std::map<UString, int> alienFunctionMap = {
    {"Alien Farm", 3},        {"Control Chamber", 2},   {"Dimension Gate Generator", 5},
    {"Food Chamber", 4},      {"Incubator Chamber", 6}, {"Maintenance Factory", 7},
    {"Megapod Chamber", 1},   {"Organic Factory", 8},   {"Sleeping Chamber", 9},
    {"Spawning Chamber", 10},
};
}

BattleBriefing::BattleBriefing(sp<GameState> state, StateRef<Organisation> targetOrg,
                               UString location, bool isBuilding, bool isRaid,
                               std::future<void> gameStateTask)
    : Stage(), menuform(ui().getForm("battle/briefing")), loading_task(std::move(gameStateTask)),
      state(state)
{

	menuform->findControlTyped<Label>("TEXT_DATE")
	    ->setText(format("%s      %s", state->gameTime.getLongDateString(),
	                     state->gameTime.getShortTimeString()));

	// FIXME: Introduce breifing text
	UString briefing = "";
	if (!isBuilding)
	{
		menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
		    ->setImage(fw().data->loadImage("xcom3/tacdata/brief3.pcx"));
		briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: UFO Assault)";
	}
	else
	{
		auto building = StateRef<Building>(&*state, location);
		if (!isRaid)
		{
			menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
			    ->setImage(fw().data->loadImage("xcom3/tacdata/brief.pcx"));
			briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: Building Alien "
			           "Extermination)";
		}
		else
		{
			if (building->owner == state->getPlayer())
			{
				menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
				    ->setImage(fw().data->loadImage("xcom3/tacdata/brief4.pcx"));
				bool lastBase = true;
				briefing = lastBase ? "You must lorem ipisum etc. (Here be briefing text) (Text: "
				                      "Building Last Base Assault)"
				                    : "You must lorem ipisum etc. (Here be briefing text) (Text: "
				                      "Building Base Assault)";
			}
			else if (building->owner != state->getAliens())
			{
				menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
				    ->setImage(fw().data->loadImage("xcom3/tacdata/brief2.pcx"));
				briefing =
				    "You must lorem ipisum etc. (Here be briefing text) (Text: Building Raid)";
			}
			else
			{
				int briefingID = alienFunctionMap.at(building->function);
				menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")
				    ->setImage(
				        fw().data->loadImage(format("xcom3/tacdata/alienm%d.pcx", briefingID)));
				switch (briefingID)
				{
					case 1:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Megapod Chamber)";
						break;
					case 2:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Control Chamber)";
						break;
					case 3:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Alien Farm)";
						break;
					case 4:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Food Chamber)";
						break;
					case 5:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Dimension Gate Generator)";
						break;
					case 6:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Incubator Chamber)";
						break;
					case 7:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Maintenance Factory)";
						break;
					case 8:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Organic Factory)";
						break;
					case 9:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Sleeping Chamber)";
						break;
					case 10:
						briefing = "You must lorem ipisum etc. (Here be briefing text) (Text: "
						           "Alien Building Spawning Chamber)";
						break;
				}
			}
		}
	}
	menuform->findControlTyped<Label>("TEXT_BRIEFING")->setText(briefing);

	// menuform->findControlTyped<Graphic>("BRIEFING_IMAGE")->setImage();
	menuform->findControlTyped<Label>("TEXT_BRIEFING")
	    ->setText("You must lorem ipisum etc. (Here be briefing text)");

	menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")->setVisible(false);
	menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")->setVisible(false);

	menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_battle->setMode(Battle::Mode::RealTime);
		    fw().stageQueueCommand(
		        {StageCmd::Command::REPLACEALL, mksp<BattlePreStart>(this->state)});
		});

	menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")
	    ->addCallback(FormEventType::ButtonClick, [this](Event *) {
		    this->state->current_battle->setMode(Battle::Mode::TurnBased);
		    fw().stageQueueCommand(
		        {StageCmd::Command::REPLACEALL, mksp<BattlePreStart>(this->state)});
		});
}

void BattleBriefing::begin() {}

void BattleBriefing::pause() {}

void BattleBriefing::resume() {}

void BattleBriefing::finish() {}

void BattleBriefing::eventOccurred(Event *e) { menuform->eventOccured(e); }

void BattleBriefing::update()
{
	menuform->update();

	auto status = this->loading_task.wait_for(std::chrono::seconds(0));
	switch (status)
	{
		case std::future_status::ready:
		{
			menuform->findControlTyped<GraphicButton>("BUTTON_REAL_TIME")->setVisible(true);
			menuform->findControlTyped<GraphicButton>("BUTTON_TURN_BASED")->setVisible(true);
		}
			return;
		default:
			// Not yet finished
			return;
	}
}

void BattleBriefing::render()
{
	menuform->render();
	// FIXME: Draw "loading" in the bottom ?
}

bool BattleBriefing::isTransition() { return false; }

}; // namespace OpenApoc
