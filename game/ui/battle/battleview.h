#pragma once

#include "game/state/battle/battleunit.h"
#include "game/ui/tileview/battletileview.h"
#include "library/colour.h"
#include "library/sp.h"

namespace OpenApoc
{

class Form;
class GameState;
class GraphicButton;
class Control;
class AEquipment;
class AEquipmentType;
class Graphic;

enum class BattleUpdateSpeed
{
	Pause,
	Speed1,
	Speed2,
	Speed3,
};

enum class BattleSelectionState
{
	Normal,
	NormalAlt,
	NormalCtrl,
	NormalCtrlAlt,
	PsiControl,
	PsiPanic,
	PsiStun,
	PsiProbe,
	FireAny,
	FireLeft,
	FireRight,
	ThrowLeft,
	ThrowRight,
	TeleportLeft,
	TeleportRight,
};

// All the info required to draw a single item info chunk, kept together to make it easier to
// track when something has changed and requires a re-draw
class AgentEquipmentInfo
{
  public:
	StateRef<AEquipmentType> itemType;
	StateRef<DamageType> damageType;
	bool selected = false;
	int curAmmo = 0;
	int maxAmmo = 0;
	int accuracy = 0;
	bool operator==(const AgentEquipmentInfo &other) const;
	bool operator!=(const AgentEquipmentInfo &other) const;
};

// All the info required to draw psi page, kept together to make it easier to
// track when something has changed and requires a re-draw
class AgentPsiInfo
{
  public:
	PsiStatus status;
	int maxEnergy = 0;
	int maxAttack = 0;
	int maxDefense = 0;
	int curEnergy = 0;
	int curAttack = 0;
	int curDefense = 0;
	bool operator==(const AgentPsiInfo &other) const;
	bool operator!=(const AgentPsiInfo &other) const;
};

class MotionScannerInfo
{
  public:
	// 0 = up, then clockwise
    int direction = 0;
	UString id = "";
	unsigned long version = 0;

	bool operator==(const MotionScannerInfo &other) const;
	bool operator!=(const MotionScannerInfo &other) const;
};

class BattleView : public BattleTileView
{
  private:
	const std::vector<Colour> accuracyColors = {
	    {190, 36, 36},  {203, 28, 2},    {235, 77, 4},    {239, 125, 52},
	    {243, 170, 85}, {247, 219, 117}, {235, 247, 138},
	};
	const Colour ammoColour = {158, 24, 12};

	sp<Form> activeTab, mainTab, psiTab, primingTab, baseForm;
	std::vector<sp<Form>> uiTabsRT;
	std::vector<sp<Form>> uiTabsTB;
	BattleUpdateSpeed updateSpeed;
	BattleUpdateSpeed lastSpeed;

	std::list<sp<Form>> itemForms;
	std::map<bool, sp<Form>> motionScannerForms;
	std::map<bool, sp<Form>> medikitForms;
	// right/left, bodypart, healing or wounded ( false = wounded, true = healing)
	std::map<bool, std::map<BodyPart, std::map<bool, sp<Control>>>> medikitBodyParts;
	std::map<bool, sp<Graphic>> motionScannerData;
	std::map<bool, sp<Graphic>> motionScannerUnit;

	sp<GameState> state;

	Battle &battle;

	AgentEquipmentInfo leftHandInfo;
	AgentEquipmentInfo rightHandInfo;
	AgentPsiInfo psiInfo;
	MotionScannerInfo motionInfo;

	bool followAgent = false;

	bool colorForward = true;
	int colorCurrent = 0;
	sp<Palette> palette;
	std::vector<sp<Palette>> modPalette;

	BattleSelectionState selectionState;
	bool modifierLShift = false;
	bool modifierRShift = false;
	bool modifierLAlt = false;
	bool modifierRAlt = false;
	bool modifierLCtrl = false;
	bool modifierRCtrl = false;
	int leftThrowDelay = 0;
	int rightThrowDelay = 0;
	int actionImpossibleDelay = 0;

	void updateSelectionMode();
	void updateSelectedUnits();
	void updateLayerButtons();
	void updateSoldierButtons();

	AgentEquipmentInfo createItemOverlayInfo(bool rightHand);
	AgentPsiInfo createPsiInfo();
	MotionScannerInfo createMotionInfo(BattleScanner &scanner);
	void updateItemInfo(bool right);
	void updatePsiInfo();
	void updateMotionInfo(bool right, BattleScanner &scanner);
	sp<RGBImage> drawPsiBar(int cur, int max);
	sp<RGBImage> drawMotionScanner(BattleScanner &scanner);
	sp<Image> selectedItemOverlay;
	sp<Image> selectedPsiOverlay;
	std::vector<sp<Image>> motionScannerDirectionIcons;

	sp<Image> pauseIcon;
	int pauseIconTimer = 0;

	void onNewTurn();

	// Unit orers

	void orderMove(Vec3<int> target, bool strafe = false, bool demandGiveWay = false);
	void orderJump(Vec3<int> target, BodyState bodyState = BodyState::Standing);
	void orderJump(Vec3<float> target, BodyState bodyState = BodyState::Standing);
	void orderTurn(Vec3<int> target);
	void orderUse(bool right, bool automatic);
	void orderDrop(bool right);
	void orderThrow(Vec3<int> target, bool right);
	void orderTeleport(Vec3<int> target, bool right);
	void orderCancelPsi();
	void orderPsiAttack(StateRef<BattleUnit> u, PsiStatus status, bool right);
	void orderSelect(StateRef<BattleUnit> u, bool inverse = false, bool additive = false);
	void orderFire(Vec3<int> target, WeaponStatus status = WeaponStatus::FiringBothHands,
	               bool modifier = false);
	void orderFire(StateRef<BattleUnit> u, WeaponStatus status = WeaponStatus::FiringBothHands);
	void orderFocus(StateRef<BattleUnit> u);
	void orderHeal(BodyPart part);

  public:
	BattleView(sp<GameState> gameState);
	~BattleView() override;
	void begin() override;
	void resume() override;
	void update() override;
	void render() override;
	void finish() override;
	void eventOccurred(Event *e) override;

	void setUpdateSpeed(BattleUpdateSpeed updateSpeed);
};

}; // namespace OpenApoc
