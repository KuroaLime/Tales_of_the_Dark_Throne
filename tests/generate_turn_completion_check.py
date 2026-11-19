"""Compile-evaluate the actual turn-completion bodies with constexpr UI/character doubles.

The extracted production queue algorithms and handler are not reimplemented.
Only their external dependencies are replaced. No native test process runs.
"""
from pathlib import Path
import argparse
from verify_character_rendering import body_of, require

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/'Tower_Triumph/Tower_Triumph'
battle=(SRC/'Battle_Stage.cpp').read_text(encoding='utf-8-sig')
queue=(SRC/'STurnTimeline.cpp').read_text(encoding='utf-8-sig')
space=battle.split('if (wParam == VK_SPACE)',1)[1].split('if (wParam == VK_LEFT',1)[0]
require('OnMenuEndTurn();' in space and 'NextTurn(' not in space and 'ApplyTileEffect(' not in space,
        'Space must dispatch to the same handler as the menu')
end=body_of(battle,'OnMenuEndTurn')
require(end.index('IsCurrentTurnEnemy()') < end.index('SoundManager::PlaySFX') < end.index('ApplyTileEffect(current);'),
        'Busy/enemy validation must precede mutations')

prefix=r'''
#include <vector>
#include <algorithm>
enum class EElement { None, Fire };
enum class ETileType { Normal, CursedGround, MagicHazard, Lava };
enum class EBattleControlMode { WAIT_COMMAND, SELECT_SKILL };
struct Location { int x=0,y=0; };
constexpr unsigned long RGB(int r,int g,int b){return r|(g<<8)|(b<<16);}
struct Character
{
    int hp=100,resets=0;
    constexpr bool IsAlive()const{return hp>0;}
    constexpr Location GetGridPos()const{return {};}
    constexpr EElement GetElement()const{return EElement::None;}
    constexpr void GetDamage(int damage){hp-=damage;if(hp<0)hp=0;}
    constexpr void ResetTurnState(){++resets;}
};
struct TurnEntry { Character* character;bool isEnemy; };
struct STurnTimeline
{
    std::vector<TurnEntry> turnOrder;
    constexpr Character* GetCurrentTurnCharacter()const{return turnOrder.empty()?nullptr:turnOrder[0].character;}
    constexpr bool IsCurrentTurnEnemy()const{return !turnOrder.empty()&&turnOrder[0].isEnemy;}
    constexpr void NextTurn();
    constexpr void RemoveDeadUnits();
};
namespace SoundManager { constexpr void PlaySFX(const wchar_t*){} }
struct Menu { bool visible=true;constexpr void SetVisible(bool value){visible=value;} };
struct Battle_Stage
{
    bool BattleEnded=false,finishOnDeath=false;
    STurnTimeline* turnSystem=nullptr;
    Character* MovingUnit=nullptr;
    Character* ActingUnit=nullptr;
    struct Pending { Character* actor=nullptr; } PendingAction;
    std::vector<Character*> members;
    Menu* actionMenu=nullptr;
    EBattleControlMode controlMode=EBattleControlMode::SELECT_SKILL;
    ETileType tile=ETileType::Normal;
    int tileDamage=0,moveClears=0,skillClears=0,damageTexts=0;
    constexpr bool ContainsBattleUnit(const Character* unit)const
    {return unit && std::find(members.begin(),members.end(),unit)!=members.end();}
    constexpr void ClearMovePath(){++moveClears;}
    constexpr void ClearBattleSkillSelection(){++skillClears;}
    constexpr bool IsDamageTile(int,int)const{return tile!=ETileType::Normal;}
    constexpr ETileType GetTileType(int,int)const{return tile;}
    constexpr int GetTileDamage(int,int)const{return tileDamage;}
    constexpr void AddDamageText(Character*,int,unsigned long){++damageTexts;}
    constexpr void CheckBattleEnd(){if(finishOnDeath && !members[0]->IsAlive())BattleEnded=true;}
    constexpr void ClearPendingAction(){PendingAction.actor=nullptr;}
    constexpr void OnMenuEndTurn();
    constexpr void ApplyTileEffect(Character* unit);
    constexpr void ResetTurnActions();
};
'''
bodies='\n'.join([
    'constexpr void STurnTimeline::NextTurn(){'+body_of(queue,'NextTurn')+'}',
    'constexpr void STurnTimeline::RemoveDeadUnits(){'+body_of(queue,'RemoveDeadUnits')+'}',
    'constexpr void Battle_Stage::ApplyTileEffect(Character* unit){'+body_of(battle,'ApplyTileEffect')+'}',
    'constexpr void Battle_Stage::ResetTurnActions(){'+body_of(battle,'ResetTurnActions')+'}',
    'constexpr void Battle_Stage::OnMenuEndTurn(){'+end+'}',
])
checks=r'''
constexpr bool CompletesTurn(bool lethal,bool endsBattle)
{
    Character a{lethal?5:20},b,c;
    STurnTimeline queue{{{&a,false},{&b,false},{&c,true}}};Menu menu;
    Battle_Stage stage;
    stage.turnSystem=&queue;stage.members={&a,&b,&c};stage.actionMenu=&menu;
    stage.tile=ETileType::MagicHazard;stage.tileDamage=8;stage.finishOnDeath=endsBattle;
    stage.OnMenuEndTurn();
    if(stage.damageTexts!=1 || stage.moveClears!=1 || stage.skillClears!=1 || menu.visible)return false;
    if(queue.GetCurrentTurnCharacter()!=&b || queue.turnOrder.size()!=(lethal?2u:3u))return false;
    if(!lethal && (a.hp!=12 || queue.turnOrder.back().character!=&a))return false;
    if(lethal && a.hp!=0)return false;
    if(endsBattle)return stage.BattleEnded && b.resets==0;
    return b.resets==1 && c.resets==0 && stage.controlMode==EBattleControlMode::WAIT_COMMAND;
}
constexpr bool Guarded(int kind)
{
    Character a,b,c;
    STurnTimeline queue{{{&a,kind==0},{&b,false},{&c,true}}};Menu menu;
    Battle_Stage stage;stage.turnSystem=&queue;stage.members={&a,&b,&c};stage.actionMenu=&menu;
    stage.tile=ETileType::MagicHazard;stage.tileDamage=8;
    if(kind==1)stage.MovingUnit=&a;
    if(kind==2)stage.ActingUnit=&a;
    if(kind==3)stage.PendingAction.actor=&a;
    if(kind==4)stage.BattleEnded=true;
    if(kind==5)stage.turnSystem=nullptr;
    if(kind==6)stage.members={&b,&c};
    if(kind==7)a.hp=0;
    stage.OnMenuEndTurn();
    return queue.GetCurrentTurnCharacter()==&a && queue.turnOrder.size()==3 &&
        stage.moveClears==0 && stage.skillClears==0 && stage.damageTexts==0 && menu.visible && b.resets==0;
}
static_assert(CompletesTurn(false,false),"A surviving actor rotates once and the next character starts normally");
static_assert(CompletesTurn(true,false),"A lethal hazard removes only the completed actor; the next living actor is not skipped");
static_assert(CompletesTurn(true,true),"A battle-ending hazard stops before a fresh turn is reset");
static_assert(Guarded(0)&&Guarded(1)&&Guarded(2)&&Guarded(3)&&Guarded(4)&&Guarded(5)&&Guarded(6)&&Guarded(7),
    "Enemy, movement, action, pending impact, finished, missing timeline, stale and dead callers cannot end another turn");
'''
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,default=ROOT/'tests/bin/turn_completion_compile.cpp')
output=parser.parse_args().output
output.parent.mkdir(parents=True,exist_ok=True)
output.write_text(prefix+bodies+checks,encoding='utf-8')
print('Prepared compile-time regression fixture from five actual production function bodies; Space/menu dispatch verified.')
