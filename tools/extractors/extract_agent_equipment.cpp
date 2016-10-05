#include "framework/framework.h"
#include "game/state/agent.h"
#include "game/state/battle/battlecommonimagelist.h"
#include "game/state/rules/aequipment_type.h"
#include "game/state/rules/damage.h"
#include "game/state/rules/doodad_type.h"
#include "tools/extractors/common/tacp.h"
#include "tools/extractors/extractors.h"
#include <limits>

#define E_TRONLAUN 37  // extra / tronlaun
#define A_QUEENWHP 97  // tactical / aliens / attacks / queenwhp
#define A_SPITTER 98   // tactical / aliens / attacks / spitter
#define A_WORMSPIT 99  // tactical / aliens / attacks / wormspit
#define A_WRMATTAK 100 // tactical / aliens / attacks / wrmattak
#define W_BULLET1 161  // bullet1
#define W_BULLET2 162  // bullet2
#define W_BULLET3 163  // bullet3
#define W_DCANNON1 164 // dcannon1
#define W_DIMNMISL 165 // dimnmisl
#define W_DISRUPTR 166 // disruptr
#define W_ENTROPY 168  // entropy
#define W_MARSEC1 169  // marsec1
#define W_MARSEC2 170  // marsec2
#define W_MEGAPOL 171  // megapol
#define W_MEGASTUN 172 // megastun
#define W_MEGCANON 173 // megcanon
#define W_MEGHIT 174   // meghit
#define W_POWERS 175   // powers
#define W_SNIPER 177   // sniper
#define W_TOXIGUN 181  // toxigun
#define W_TRAKGUN 182  // trakgun
#define W_TRAKHIT 183  // trakgun
#define W_ZAPHIT 184   // zaphit

// Doodads used by tactical weaponry, ids matching vanilla
#define TAC_DOODAD_21 21 // tac 115 - 125
#define TAC_DOODAD_22 22 // tac 126 - 136
#define TAC_DOODAD_23 23 // tac 137 - 147
#define TAC_DOODAD_24 24 // tac 148 - 158
#define TAC_DOODAD_25 25 // tac 159 - 169
#define TAC_DOODAD_26 26 // tac 170 - 180
#define TAC_DOODAD_27 27 // tac 181 - 185 shield
#define TAC_DOODAD_28 28 // tac 186 - 192

// Doodads matching their vanilla indexes
#define TAC_DOODAD_18 18 // tac 44, 52
#define TAC_DOODAD_19 19 // tac 52, 60
#define TAC_DOODAD_20 20 // tac 60, 68

// Doodads defined manually, ids not matching vanilla
#define CUSTOM_DOODAD_16 16 //tac 28, 32
#define CUSTOM_DOODAD_17 17// tac 32, 44
#define CUSTOM_DOODAD_29 29// tac 8, 26

namespace OpenApoc
{

void InitialGameStateExtractor::extractAgentEquipment(GameState &state, Difficulty difficulty)
{
	auto &data_t = this->tacp;
	auto &data_u = this->ufo2p;

	auto gameObjectSpriteTabFileName = UString("xcom3/tacdata/gameobj.tab");
	auto gameObjectSpriteTabFile = fw().data->fs.open(gameObjectSpriteTabFileName);
	if (!gameObjectSpriteTabFile)
	{
		LogError("Failed to open dropped item sprite TAB file \"%s\"",
		         gameObjectSpriteTabFileName.cStr());
		return;
	}
	size_t gameObjectSpriteCount = gameObjectSpriteTabFile.size() / 4;

	auto gameObjectShadowSpriteTabFileName = UString("xcom3/tacdata/oshadow.tab");
	auto gameObjectShadowSpriteTabFile = fw().data->fs.open(gameObjectShadowSpriteTabFileName);
	if (!gameObjectShadowSpriteTabFile)
	{
		LogError("Failed to open shadow dropped item sprite TAB file \"%s\"",
		         gameObjectShadowSpriteTabFileName.cStr());
		return;
	}
	size_t gameObjectShadowSpriteCount = gameObjectShadowSpriteTabFile.size() / 4;

	auto heldSpriteTabFileName = UString("xcom3/tacdata/unit/equip.tab");
	auto heldSpriteTabFile = fw().data->fs.open(heldSpriteTabFileName);
	if (!heldSpriteTabFile)
	{
		LogError("Failed to open held item sprite TAB file \"%s\"", heldSpriteTabFileName.cStr());
		return;
	}
	size_t heldSpriteCount = heldSpriteTabFile.size() / 4 / 8;

	std::map<int, sp<AEquipmentType>> weapons;
	UString tracker_gun_clip_id = "";

	for (unsigned i = 0; i < data_t.damage_type_names->count(); i++)
	{
		auto d = mksp<DamageType>();

		d->name = data_t.damage_type_names->get(i);
		UString id = data_t.getDTypeId(i);

		d->id = id;

		d->ignore_shield =
		    (i < data_t.damage_types->count()) && (data_t.damage_types->get(i).ignore_shield == 1);

		// Damage icons are located in tacdata icons, starting with id 14 and on
		d->icon_sprite =
		    fw().data->loadImage(UString::format("PCK:xcom3/tacdata/icons.pck:xcom3/tacdata/"
		                                         "icons.tab:%d:xcom3/tacdata/tactical.pal",
		                                         (int)i + 14));

		state.damage_types[id] = d;
	}

	for (unsigned i = 0; i < data_t.damage_modifiers->count(); i++)
	{
		auto d = mksp<DamageModifier>();
		auto ddata = data_t.damage_modifiers->get(i);

		d->name = data_t.damage_modifier_names->get(i);
		UString id = data_t.getDModId(i);

		d->id = id;

		state.damage_modifiers[id] = d;

		for (unsigned j = 0; j < 18; j++)
		{
			state.damage_types[data_t.getDTypeId(j)]->modifiers[{&state, id}] =
			    ddata.damage_type_data[j];
		}
	}

	for (unsigned i = 0; i < data_t.agent_equipment->count(); i++)
	{
		auto e = mksp<AEquipmentType>();
		auto edata = data_t.agent_equipment->get(i);

		e->name = data_u.agent_equipment_names->get(i);
		UString id = UString::format("%s%s", AEquipmentType::getPrefix(), canon_string(e->name));

		e->id = id;

		// Mark two-handed items
		switch (edata.sprite_idx)
		{
			// Minigun
			case 7:
			// Laser Sniper
			case 9:
			// Autocannon
			case 11:
			// Heavy Launcher
			case 17:
			// MiniLauncher
			case 21:
			// Stun Grapple
			case 25:
			// Tracker Gun
			case 28:
			// ForceWeb
			case 31:
			// Devastator Canon
			case 41:
			// Brainsucker
			case 44:
			// Dimension Missile Launcher
			case 46:
				e->two_handed = true;
				break;
			default:
				e->two_handed = false;
				break;
		}

		unsigned payload_idx = std::numeric_limits<unsigned>::max();
		switch (edata.type)
		{
			case AGENT_EQUIPMENT_TYPE_ARMOR:
			{
				auto adata = data_t.agent_armor->get(edata.data_idx);
				e->type = AEquipmentType::Type::Armor;
				e->damage_modifier = {&state, data_t.getDModId(adata.damage_modifier)};
				e->armor = adata.armor;
				e->max_ammo = adata.armor;
				UString bodyPartLetter = "";
				int armoredUnitPicIndex = 0;
				int armorBodyPicIndex = 0;
				switch (adata.body_part)
				{
					case AGENT_ARMOR_BODY_PART_LEGS:
						bodyPartLetter = "b";
						armorBodyPicIndex = 4;
						e->body_part = AgentType::BodyPart::Legs;
						break;
					case AGENT_ARMOR_BODY_PART_BODY:
						bodyPartLetter = "a";
						armorBodyPicIndex = 2;
						e->body_part = AgentType::BodyPart::Body;
						// Vanilla decides if flight is enabled by checking if a "marsec" damage mod
						// body armor is equipped
						if (adata.damage_modifier == 18)
							e->provides_flight = true;
						break;
					case AGENT_ARMOR_BODY_PART_LEFT_ARM:
						bodyPartLetter = "d";
						armorBodyPicIndex = 1;
						e->body_part = AgentType::BodyPart::LeftArm;
						break;
					case AGENT_ARMOR_BODY_PART_RIGHT_ARM:
						bodyPartLetter = "e";
						armorBodyPicIndex = 3;
						e->body_part = AgentType::BodyPart::RightArm;
						break;
					case AGENT_ARMOR_BODY_PART_HELMET:
						bodyPartLetter = "c";
						armorBodyPicIndex = 0;
						e->body_part = AgentType::BodyPart::Helmet;
						break;
					default:
						LogError("Unexpected body part type %d for ID %s", (int)adata.body_part,
						         id.cStr());
				}
				switch (adata.damage_modifier)
				{
					case 17:
						armoredUnitPicIndex = 2;
						break;
					case 18:
						armoredUnitPicIndex = 3;
						break;
					case 19:
						armoredUnitPicIndex = 4;
						break;
					default:
						LogError("Unexpected damage modifier %d for ID %s",
						         (int)adata.damage_modifier, id.cStr());
						break;
				}
				e->body_image_pack = {&state,
				                      UString::format("%s%s%d%s", BattleUnitImagePack::getPrefix(),
				                                      "xcom", armoredUnitPicIndex, bodyPartLetter)};
				// Body sprites are stored in armour.pck file, in head-left-body-right-legs order
				// Since armor damage modifier values start with 17, we can subtract that to get
				// armor index
				e->body_sprite = fw().data->loadImage(
				    UString::format("PCK:xcom3/ufodata/armour.pck:xcom3/ufodata/"
				                    "armour.tab:%d:xcom3/tacdata/equip.pal",
				                    (int)((adata.damage_modifier - 17) * 5 + armorBodyPicIndex)));
			}
			break;
			case AGENT_EQUIPMENT_TYPE_WEAPON:
			{
				auto wdata = data_t.agent_weapons->get(edata.data_idx);
				if (wdata.ammo_effect[0] == 255)
				{
					e->type = AEquipmentType::Type::Grenade;
					payload_idx = wdata.grenade_effect;
					e->max_ammo = 1;
					e->recharge = 0;
				}
				else
				{
					e->type = AEquipmentType::Type::Weapon;
					bool has_ammo = false;
					// Entry #0 is "EMPTY" so we skip it
					for (unsigned j = 1; j < data_t.agent_general->count(); j++)
					{
						if (has_ammo)
							continue;
						auto gdata = data_t.agent_general->get(j);
						// Fix for buggy tracer gun ammo
						if (gdata.ammo_type == 0xffff && gdata.ammo_type_duplicate == 1)
						{
							if (wdata.ammo_type == 8)
								has_ammo = true;
							continue;
						}
						// Look for a clip with matching ammo type
						if ((gdata.ammo_type != 0xffff
						         ? gdata.ammo_type
						         : gdata.ammo_type_duplicate) == wdata.ammo_type)
							has_ammo = true;
					}

					if (has_ammo)
					{
						// All proper weapons have ammo types defined after them, so we can add ammo
						// when we parse it
						weapons[wdata.ammo_type] = e;
					}
					else
					{
						payload_idx = wdata.ammo_effect[0];
						e->max_ammo = wdata.ammo_rounds[0];
						// Alien weapons start from index 29, and are not marked as recharging
						// despite doing so. Therefore, mark them manually
						e->recharge = (edata.data_idx > 28) ? 1 : wdata.ammo_recharge[0];
					}
				}
			}
			break;
			case AGENT_EQUIPMENT_TYPE_GENERAL:
			{
				auto gdata = data_t.agent_general->get(edata.data_idx);
				switch (gdata.type)
				{
					case AGENT_GENERAL_TYPE_AMMO_OR_LOOT:
						if (gdata.ammo_effect == 255)
						{
							e->type = AEquipmentType::Type::Loot;
						}
						else
						{
							e->type = AEquipmentType::Type::Ammo;
							payload_idx = gdata.ammo_effect;
							e->max_ammo = gdata.ammo_rounds;
							e->recharge = gdata.ammo_recharge;
							if (gdata.ammo_type == 65535 && gdata.ammo_type_duplicate == 1)
								// Buggy tracker gun clip has incorrect values in these fields AND
								// comes before it's weapon!
								tracker_gun_clip_id = id;
							else
								// Behave normally
								e->weapon_types.emplace(
								    &state,
								    weapons[gdata.ammo_type != 0xffff ? gdata.ammo_type
								                                      : gdata.ammo_type_duplicate]
								        ->id);
						}
						break;
					case AGENT_GENERAL_TYPE_MOTION_SCANNER:
						e->type = AEquipmentType::Type::MotionScanner;
						break;
					case AGENT_GENERAL_TYPE_STRUCTURE_PROBE:
						e->type = AEquipmentType::Type::StructureProbe;
						break;
					case AGENT_GENERAL_TYPE_VORTEX_ANALYZER:
						e->type = AEquipmentType::Type::VortexAnalyzer;
						break;
					case AGENT_GENERAL_TYPE_MULTI_TRACKER:
						e->type = AEquipmentType::Type::MultiTracker;
						break;
					case AGENT_GENERAL_TYPE_MIND_SHIELD:
						e->type = AEquipmentType::Type::MindShield;
						break;
					case AGENT_GENERAL_TYPE_MIND_BENDER:
						e->type = AEquipmentType::Type::MindBender;
						break;
					case AGENT_GENERAL_TYPE_ALIEN_DETECTOR:
						e->type = AEquipmentType::Type::AlienDetector;
						break;
					case AGENT_GENERAL_TYPE_DISRUPTOR_SHIELD:
						e->type = AEquipmentType::Type::DisruptorShield;
						e->max_ammo = 100;
						e->recharge = 1;
						break;
					case AGENT_GENERAL_TYPE_TELEPORTER:
						e->type = AEquipmentType::Type::Teleporter;
						e->max_ammo = gdata.ammo_rounds;
						e->recharge = gdata.ammo_recharge;
						break;
					case AGENT_GENERAL_TYPE_CLOAKING_FIELD:
						e->type = AEquipmentType::Type::CloakingField;
						break;
					case AGENT_GENERAL_TYPE_DIMENSION_FORCE_FIELD:
						e->type = AEquipmentType::Type::DimensionForceField;
						break;
					case AGENT_GENERAL_TYPE_MEDI_KIT:
						e->type = AEquipmentType::Type::MediKit;
						break;
					default:
						LogError("Unexpected general type %d for ID %s", (int)gdata.type,
						         id.cStr());
				}
			}
			break;
			default:
				LogInfo("Encountered empty item in ID %s, moving on", id.cStr());
				continue;
		}

		e->weight = edata.weight;

		e->shadow_offset = BATTLE_SHADOW_OFFSET;
		e->dropped_offset = BATTLE_IMAGE_OFFSET;

		if (edata.sprite_idx < gameObjectSpriteCount)
			e->dropped_sprite =
			    fw().data->loadImage(UString::format("PCK:xcom3/tacdata/gameobj.pck:xcom3/tacdata/"
			                                         "gameobj.tab:%d",
			                                         (int)edata.sprite_idx));

		if (edata.sprite_idx < gameObjectShadowSpriteCount)
			e->dropped_shadow_sprite = fw().data->loadImage(
			    UString::format("PCKSHADOW:xcom3/tacdata/oshadow.pck:xcom3/tacdata/"
			                    "oshadow.tab:%d",
			                    (int)edata.sprite_idx));

		// Held sprites begin from 0, which corresponds to item 1, Megapol AP Grenade
		// Armor pieces go last, and held sprites for every single item after the first armor piece
		// are identical
		// There is a total 60 of them
		int held_sprite_index = std::min((int)edata.sprite_idx, (int)heldSpriteCount - 1);
		e->held_image_pack = {&state, UString::format("%s%s%d", BattleUnitImagePack::getPrefix(),
		                                              "item", held_sprite_index)};

		e->equipscreen_sprite = fw().data->loadImage(UString::format(
		    "PCK:xcom3/ufodata/pequip.pck:xcom3/ufodata/pequip.tab:%d:xcom3/tacdata/tactical.pal",
		    (int)edata.sprite_idx));
		e->equipscreen_size = {edata.size_x, edata.size_y};

		e->manufacturer = {&state, data_u.getOrgId(edata.manufacturer)};

		e->store_space = edata.store_space;
		e->armor = edata.armor;
		e->score = edata.score;

		// Read payload information
		// Only for Ammo, or weapons and grenades with built-in ammo
		if (payload_idx != std::numeric_limits<unsigned>::max())
		{
			if (payload_idx >= data_t.agent_payload->count())
				LogError("Invalid payload index %u for ID %s", payload_idx, id.cStr());

			auto pdata = data_t.agent_payload->get(payload_idx);

			e->speed = pdata.speed;
			e->damage = pdata.damage;
			e->accuracy = 100 - pdata.accuracy;
			e->fire_delay = pdata.fire_delay;
			e->guided = pdata.guided != 0 ? true : false;
			e->turn_rate = pdata.turn_rate;
			e->ttl = pdata.ttl;
			e->range = pdata.range;
			UString doodad_id = "";
			switch (pdata.explosion_graphic)
			{
				case TAC_DOODAD_21: // tac 115 - 125
					doodad_id = "DOODAD_21_AP";
					break;
				case TAC_DOODAD_22: // tac 126 - 136
					doodad_id = "DOODAD_22_LASER";
					break;
				case TAC_DOODAD_23: // tac 137 - 147
					doodad_id = "DOODAD_23_PLASMA";
					break;
				case TAC_DOODAD_24: // tac 148 - 158
					doodad_id = "DOODAD_24_DISRUPTOR";
					break;
				case TAC_DOODAD_25: // tac 159 - 169
					doodad_id = "DOODAD_25_DEVASTATOR";
					break;
				case TAC_DOODAD_26: // tac 170 - 180
					doodad_id = "DOODAD_26_STUN";
					break;
				case TAC_DOODAD_27: // tac 181 - 185 shield
					doodad_id = "DOODAD_27_SHIELD";
					break;
				case TAC_DOODAD_28: // tac 186 - 192
					doodad_id = "DOODAD_28_ENZYME";
					break;
			}
			if (doodad_id != "")
			{
				e->explosion_graphic = {&state, doodad_id};
			}

			UString fire_sfx_path = "";
			switch (pdata.fire_sfx)
			{
				case E_TRONLAUN:
					fire_sfx_path = "extra/tronlaun";
					break;
				case A_QUEENWHP:
					fire_sfx_path = "tactical/aliens/attacks/queenwhp";
					break;
				case A_SPITTER:
					fire_sfx_path = "tactical/aliens/attacks/spitter";
					break;
				case A_WORMSPIT:
					fire_sfx_path = "tactical/aliens/attacks/wormspit";
					break;
				case A_WRMATTAK:
					fire_sfx_path = "tactical/aliens/attacks/wrmattak";
					break;
				case W_BULLET1:
					fire_sfx_path = "tactical/weapons/bullet1";
					break;
				case W_BULLET2:
					fire_sfx_path = "tactical/weapons/bullet2";
					break;
				case W_BULLET3:
					fire_sfx_path = "tactical/weapons/bullet3";
					break;
				case W_DCANNON1:
					fire_sfx_path = "tactical/weapons/dcannon1";
					break;
				case W_DIMNMISL:
					fire_sfx_path = "tactical/weapons/dimnmisl";
					break;
				case W_DISRUPTR:
					fire_sfx_path = "tactical/weapons/disruptr";
					break;
				case W_ENTROPY:
					fire_sfx_path = "tactical/weapons/entropy";
					break;
				case W_MARSEC1:
					fire_sfx_path = "tactical/weapons/marsec1";
					break;
				case W_MARSEC2:
					fire_sfx_path = "tactical/weapons/marsec2";
					break;
				case W_MEGAPOL:
					fire_sfx_path = "tactical/weapons/megapol";
					break;
				case W_MEGASTUN:
					fire_sfx_path = "tactical/weapons/megastun";
					break;
				case W_MEGCANON:
					fire_sfx_path = "tactical/weapons/megcanon";
					break;
				case W_MEGHIT:
					fire_sfx_path = "tactical/weapons/meghit";
					break;
				case W_POWERS:
					fire_sfx_path = "tactical/weapons/powers";
					break;
				case W_SNIPER:
					fire_sfx_path = "tactical/weapons/sniper";
					break;
				case W_TOXIGUN:
					fire_sfx_path = "tactical/weapons/toxigun";
					break;
				case W_TRAKGUN:
					fire_sfx_path = "tactical/weapons/trakgun";
					break;
				case W_TRAKHIT:
					fire_sfx_path = "tactical/weapons/trakhit";
					break;
				case W_ZAPHIT:
					fire_sfx_path = "tactical/weapons/zaphit";
					break;
			}
			if (fire_sfx_path != "")
			{
				e->fire_sfx = fw().data->loadSample("RAWSOUND:xcom3/rawsound/" + fire_sfx_path +
				                                    ".raw:22050");
			}

			UString impact_sfx_path = "";
			switch (pdata.impact_sfx)
			{
				case E_TRONLAUN:
					impact_sfx_path = "extra/tronlaun";
					break;
				case A_QUEENWHP:
					impact_sfx_path = "tactical/aliens/attacks/queenwhp";
					break;
				case A_SPITTER:
					impact_sfx_path = "tactical/aliens/attacks/spitter";
					break;
				case A_WORMSPIT:
					impact_sfx_path = "tactical/aliens/attacks/wormspit";
					break;
				case A_WRMATTAK:
					impact_sfx_path = "tactical/aliens/attacks/wrmattak";
					break;
				case W_BULLET1:
					impact_sfx_path = "tactical/weapons/bullet1";
					break;
				case W_BULLET2:
					impact_sfx_path = "tactical/weapons/bullet2";
					break;
				case W_BULLET3:
					impact_sfx_path = "tactical/weapons/bullet3";
					break;
				case W_DCANNON1:
					impact_sfx_path = "tactical/weapons/dcannon1";
					break;
				case W_DIMNMISL:
					impact_sfx_path = "tactical/weapons/dimnmisl";
					break;
				case W_DISRUPTR:
					impact_sfx_path = "tactical/weapons/disruptr";
					break;
				case W_ENTROPY:
					impact_sfx_path = "tactical/weapons/entropy";
					break;
				case W_MARSEC1:
					impact_sfx_path = "tactical/weapons/marsec1";
					break;
				case W_MARSEC2:
					impact_sfx_path = "tactical/weapons/marsec2";
					break;
				case W_MEGAPOL:
					impact_sfx_path = "tactical/weapons/megapol";
					break;
				case W_MEGASTUN:
					impact_sfx_path = "tactical/weapons/megastun";
					break;
				case W_MEGCANON:
					impact_sfx_path = "tactical/weapons/megcanon";
					break;
				case W_MEGHIT:
					impact_sfx_path = "tactical/weapons/meghit";
					break;
				case W_POWERS:
					impact_sfx_path = "tactical/weapons/powers";
					break;
				case W_SNIPER:
					impact_sfx_path = "tactical/weapons/sniper";
					break;
				case W_TOXIGUN:
					impact_sfx_path = "tactical/weapons/toxigun";
					break;
				case W_TRAKGUN:
					impact_sfx_path = "tactical/weapons/trakgun";
					break;
				case W_TRAKHIT:
					impact_sfx_path = "tactical/weapons/trakhit";
					break;
				case W_ZAPHIT:
					impact_sfx_path = "tactical/weapons/zaphit";
					break;
			}
			if (impact_sfx_path != "")
			{
				e->impact_sfx = fw().data->loadSample("RAWSOUND:xcom3/rawsound/" + impact_sfx_path +
				                                      ".raw:22050");
			}

			e->damage_type = {&state, data_t.getDTypeId(pdata.damage_type)};
			switch (pdata.trigger_type)
			{
				case AGENT_GRENADE_TRIGGER_TYPE_NORMAL:
					e->trigger_type = AEquipmentType::TriggerType::Normal;
					break;
				case AGENT_GRENADE_TRIGGER_TYPE_PROXIMITY:
					e->trigger_type = AEquipmentType::TriggerType::Proximity;
					break;
				case AGENT_GRENADE_TRIGGER_TYPE_BOOMEROID:
					e->trigger_type = AEquipmentType::TriggerType::Boomeroid;
					break;
				default:
					LogError("Unexpected grenade trigger type %d for ID %s",
					         (int)pdata.trigger_type, id.cStr());
			}
			e->explosion_depletion_rate = pdata.explosion_depletion_rate;

			auto projectile_sprites = data_t.projectile_sprites->get(pdata.projectile_image);
			// Determine tail size
			e->tail_size = 0;
			for (int i = 0; i < 36; i++)
			{
				if (projectile_sprites.sprites[i] != 255)
				{
					e->tail_size = i + 1;
				}
			}

			for (int i = 0; i < e->tail_size; i++)
			{
				UString sprite_path = "";
				if (projectile_sprites.sprites[i] != 255)
				{
					sprite_path = UString::format("bulletsprites/battle/%02u.png",
					                              (unsigned)projectile_sprites.sprites[i]);
				}
				else
				{
					sprite_path = ""; // a 'gap' in the projectile trail
				}
				e->projectile_sprites.push_back(fw().data->loadImage(sprite_path));
			}
		}

		state.agent_equipment[id] = e;
	}

	// Fix the tracker gun ammo
	state.agent_equipment[tracker_gun_clip_id]->weapon_types.emplace(&state, weapons[8]->id);

	// Equipment sets - score - alien
	{
		if (data_t.agent_equipment_set_score_requirement->count() != 1)
			LogError("Incorrect amount of alien score requirement structures: encountered %u, "
			         "expected 1",
			         (unsigned)data_t.agent_equipment_set_score_requirement->count());
		auto sdata = data_t.agent_equipment_set_score_requirement->get(0);

		if (data_t.agent_equipment_set_score_alien->count() != 1)
			LogError("Incorrect amount of alien score equipment set structures: encountered %u, "
			         "expected 1",
			         (unsigned)data_t.agent_equipment_set_score_alien->count());
		auto data = data_t.agent_equipment_set_score_alien->get(0);
		for (unsigned i = 0; i < 8; i++)
		{
			auto es = mksp<EquipmentSet>();

			UString id = UString::format("%sALIEN_%d", EquipmentSet::getPrefix(), (int)i + 1);
			es->id = id;

			for (unsigned j = 0; j < 10; j++)
			{
				if (data.weapons[j][i].weapon_idx > 0)
				{
					if (data.weapons[j][i].clip_idx > 0)
					{
						es->weapons.push_back(
						    {{&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.weapons[j][i].weapon_idx)))},
						     {&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.weapons[j][i].clip_idx)))},
						     std::max((int)data.weapons[j][i].clip_amount, 1)});
					}
					else
					{
						es->weapons.push_back(
						    {{&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.weapons[j][i].weapon_idx)))}});
					}
				}
				if (data.grenades[j][i].grenade_idx > 0 && data.grenades[j][i].grenade_amount > 0)
				{
					es->grenades.push_back(
					    {{&state, UString::format("%s%s", AEquipmentType::getPrefix(),
					                              canon_string(data_u.agent_equipment_names->get(
					                                  data.grenades[j][i].grenade_idx)))},
					     data.grenades[j][i].grenade_amount});
				}
				if (data.equipment[j][i][0] > 0 || data.equipment[j][i][1] > 0)
				{
					if (data.equipment[j][i][0] > 0 && data.equipment[j][i][1] > 0)
					{
						es->equipment.push_back(
						    {{&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.equipment[j][i][0])))},
						     {&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.equipment[j][i][1])))}});
					}
					else if (data.equipment[j][i][0] > 0)
					{
						es->equipment.push_back(
						    {{&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.equipment[j][i][0])))}});
					}
					else
					{
						es->equipment.push_back(
						    {{&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.equipment[j][i][1])))}});
					}
				}
			}

			int diff = 0;
			switch (difficulty)
			{
				case Difficulty::DIFFICULTY_1:
					diff = 0;
					break;
				case Difficulty::DIFFICULTY_2:
					diff = 1;
					break;
				case Difficulty::DIFFICULTY_3:
					diff = 2;
					break;
				case Difficulty::DIFFICULTY_4:
					diff = 3;
					break;
				case Difficulty::DIFFICULTY_5:
					diff = 4;
					break;
				default:
					LogError("Unknown difficulty");
			}

			es->min_score = i == 0 ? INT_MIN : (int)sdata.score[diff][i - 1];
			es->max_score = i == 7 ? INT_MAX : (int)sdata.score[diff][i];

			state.equipment_sets_by_score[id] = es;
		}
	}
	// Equipment sets - score (level) - human
	{
		if (data_t.agent_equipment_set_score_human->count() != 1)
			LogError("Incorrect amount of human score equipment set structures: encountered %u, "
			         "expected 1",
			         (unsigned)data_t.agent_equipment_set_score_human->count());
		auto data = data_t.agent_equipment_set_score_human->get(0);
		for (unsigned i = 0; i < 12; i++)
		{
			for (unsigned i = 0; i < 12; i++)
			{
				auto es = mksp<EquipmentSet>();

				UString id = UString::format("%sHUMAN_%d", EquipmentSet::getPrefix(), (int)i + 1);
				es->id = id;

				for (unsigned j = 0; j < 10; j++)
				{
					if (data.weapons[j][i].weapon_idx > 0)
					{
						if (data.weapons[j][i].clip_idx > 0)
						{
							es->weapons.push_back(
							    {{&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.weapons[j][i].weapon_idx)))},
							     {&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.weapons[j][i].clip_idx)))},
							     std::max((int)data.weapons[j][i].clip_amount, 1)});
						}
						else
						{
							es->weapons.push_back(
							    {{&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.weapons[j][i].weapon_idx)))}});
						}
					}
					if (data.grenades[j][i].grenade_idx > 0 &&
					    data.grenades[j][i].grenade_amount > 0)
					{
						es->grenades.push_back(
						    {{&state,
						      UString::format("%s%s", AEquipmentType::getPrefix(),
						                      canon_string(data_u.agent_equipment_names->get(
						                          data.grenades[j][i].grenade_idx)))},
						     data.grenades[j][i].grenade_amount});
					}
					if (data.equipment[j][i][0] > 0 || data.equipment[j][i][1] > 0)
					{
						if (data.equipment[j][i][0] > 0 && data.equipment[j][i][1] > 0)
						{
							es->equipment.push_back(
							    {{&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.equipment[j][i][0])))},
							     {&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.equipment[j][i][1])))}});
						}
						else if (data.equipment[j][i][0] > 0)
						{
							es->equipment.push_back(
							    {{&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.equipment[j][i][0])))}});
						}
						else
						{
							es->equipment.push_back(
							    {{&state,
							      UString::format("%s%s", AEquipmentType::getPrefix(),
							                      canon_string(data_u.agent_equipment_names->get(
							                          data.equipment[j][i][1])))}});
						}
					}
				}

				es->min_score = i == 0 ? INT_MIN : i;
				es->max_score = i == 12 ? INT_MAX : i + 1;

				state.equipment_sets_by_level[id] = es;
			}
		}
	}

	// DOODADS
	{
		static const int frameTTL = 4;
		static const std::vector<Vec2<int>> doodadTabOffsets = {
			{ 28, 32 },{ 32, 44 },{ 44, 52 },{ 52, 60 }, { 60, 68 },
			{115, 126}, {126, 137}, {137, 148}, {148, 159},
		    {159, 170}, {170, 181}, {181, 186}, {186, 192},
			{ 8, 26 }
		};

		for (int i = 16; i <= 29; i++)
		{
			UString id;
			switch (i)
			{
				case CUSTOM_DOODAD_16: //tac 28, 32
					id = "DOODAD_16_BURNING_OBJECT";
					break;
				case CUSTOM_DOODAD_17:// tac 32, 44
					id = "DOODAD_17_FIRE";
					break;
				case TAC_DOODAD_18: // tac 44, 52
					id = "DOODAD_18_SMOKE";
					break;
				case TAC_DOODAD_19: // tac 52, 60
					id = "DOODAD_19_ALIEN_GAS";
					break;
				case TAC_DOODAD_20: // tac 60, 68
					id = "DOODAD_20_STUN_GAS";
					break;
				case TAC_DOODAD_21: // tac 115 - 125
					id = "DOODAD_21_AP";
					break;
				case TAC_DOODAD_22: // tac 126 - 136
					id = "DOODAD_22_LASER";
					break;
				case TAC_DOODAD_23: // tac 137 - 147
					id = "DOODAD_23_PLASMA";
					break;
				case TAC_DOODAD_24: // tac 148 - 158
					id = "DOODAD_24_DISRUPTOR";
					break;
				case TAC_DOODAD_25: // tac 159 - 169
					id = "DOODAD_25_DEVASTATOR";
					break;
				case TAC_DOODAD_26: // tac 170 - 180
					id = "DOODAD_26_STUN";
					break;
				case TAC_DOODAD_27: // tac 181 - 185 shield
					id = "DOODAD_27_SHIELD";
					break;
				case TAC_DOODAD_28: // tac 186 - 192
					id = "DOODAD_28_ENZYME";
					break;
				case CUSTOM_DOODAD_29 :// tac 8, 26
					id = "DOODAD_29_EXPLODING_TERRAIN";
					break;
			}

			auto tabOffsets = doodadTabOffsets[i - 16];
			auto d = mksp<DoodadType>();

			// For some reason, not equal to other offsets, which are 23,34?
			// d->imageOffset = { 23,32 };
			// Let's try common one
			// FIXME: ENSURE CORRECT
			d->imageOffset = BATTLE_IMAGE_OFFSET;
			d->lifetime = (tabOffsets.y - tabOffsets.x) * frameTTL;
			d->repeatable = false;
			for (int j = tabOffsets.x; j < tabOffsets.y; j++)
			{
				d->frames.push_back({fw().data->loadImage(UString::format(
				                         "PCK:xcom3/tacdata/ptang.pck:xcom3/tacdata/"
				                         "ptang.tab:%d",
				                         j)),
				                     frameTTL});
			}

			state.doodad_types[id] = d;
		}
	}
}

} // namespace OpenApoc