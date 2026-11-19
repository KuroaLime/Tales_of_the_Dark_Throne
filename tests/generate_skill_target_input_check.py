"""Extract actual battle input/camera bodies for compile-time regression checks.

Window/cursor, character, and casting dependencies are constexpr test doubles.
The production rectangle, coordinate conversion, camera, and handler bodies are
not reimplemented. This prepares a C++20 fixture; it never runs a native binary.
"""
import argparse
from pathlib import Path
import re

from verify_graveyard import require, strip_comments

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'Tower_Triumph/Tower_Triumph'
battle = (SOURCE / 'Battle_Stage.cpp').read_text(encoding='utf-8-sig')
header = (SOURCE / 'Battle_Stage.h').read_text(encoding='utf-8-sig')


def production_body(name):
    clean = strip_comments(battle)
    match = re.search(r'\bBattle_Stage::' + re.escape(name) +
                      r'\s*\([^;{}]*\)\s*(?:const\s*)?\{', clean)
    require(match is not None, f'Missing production member {name}')
    start = match.end()
    depth = 1
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|[{}]', clean[start:]):
        if token[0] == '{':
            depth += 1
        elif token[0] == '}':
            depth -= 1
            if depth == 0:
                return clean[start:start + token.start()]
    raise AssertionError(f'Unterminated production member {name}')


require('RECT GetBattleSkillPanelRect() const;' in header,
        'The shared skill panel rectangle must be declared')
for draw in ('DrawBattleSkillList', 'DrawBattleSkillTargetSelect'):
    require('RECT panel = GetBattleSkillPanelRect();' in production_body(draw),
            f'{draw}: paint and pointer interception must share the same bounds')

prefix = r'''
#include <vector>
#include <utility>
using HWND = int;
struct RECT { int left=0,top=0,right=0,bottom=0; };
struct POINT { int x=0,y=0; };
struct Location { int x=0,y=0; };
struct BuSIZE { int sX=0,sY=0; };
enum class EBattleControlMode { WAIT_COMMAND, SELECT_MOVE, SELECT_SKILL };
struct CharacterStat { int CurMP=20; };
struct Character
{
    bool alive=true;
    CharacterStat stat;
    constexpr bool IsAlive()const{return alive;}
    constexpr CharacterStat GetStat()const{return stat;}
};
struct SSkillData { int MPCost; };
constexpr SSkillData skill{6};
constexpr const SSkillData* FindBattleSkillInfo(int id)
{return id>=3301 && id<=3303 ? &skill : nullptr;}
constexpr unsigned long RGB(int r,int g,int b){return r|(g<<8)|(b<<16);}
struct Battle_Stage
{
    RECT rects{0,0,1280,720};
    int W=40,H=30,TILE_SIZE=48;
    float camPosX=960,camPosY=720;
    std::pair<Location,BuSIZE> Loc{{640,360},{960,720}};
    EBattleControlMode controlMode=EBattleControlMode::SELECT_SKILL;
    bool bSelectingBattleSkillTarget=true;
    int SelectedBattleSkillID=3301;
    std::vector<int> BattleSkillIDs{3301,3302,3303};
    int HoverGridX=15,HoverGridY=14,LastMouseX=424,LastMouseY=336;
    POINT cursor{424,336};
    bool cursorAvailable=true,conversionAvailable=true,allowTarget=true;
    Character* enemy=nullptr;
    int enemyX=15,enemyY=14,enemyQueries=0,casts=0,notices=0;
    Character* castTarget=nullptr;
    constexpr bool GetCursorPos(POINT* point)const{*point=cursor;return cursorAvailable;}
    constexpr bool ScreenToClient(HWND,POINT*)const{return conversionAvailable;}
    constexpr void RefreshBattleSkillList(Character*){BattleSkillIDs={3301,3302,3303};}
    constexpr void AddDamageText(Character*,int,unsigned long){++notices;}
    constexpr Character* GetEnemyAt(int x,int y)
    {++enemyQueries;return x==enemyX && y==enemyY ? enemy : nullptr;}
    constexpr bool IsBattleSkillTargetValid(Character*,int,int,int)const{return allowTarget;}
    constexpr bool UseSelectedBattleSkill(Character*,Character* target)
    {++casts;castTarget=target;return true;}
    constexpr RECT GetBattleSkillPanelRect()const;
    constexpr RECT GetBattleSkillRect(int index)const;
    constexpr RECT GetBattleItemRect(int index)const;
    constexpr RECT GetBattleCancelRect()const;
    constexpr bool IsPointInRect(const RECT& rc,int x,int y)const;
    constexpr void ClearBattleSkillSelection();
    constexpr void UpdateHoverGridByMouse(int mouseX,int mouseY);
    constexpr void UpdateSkillTargetCameraByMouse(HWND hWnd);
    constexpr void BackgroundMove(int x,int y);
    constexpr void FocusCameraOnWorldPos(int wx,int wy);
    constexpr int GetOffsetX()const;
    constexpr int GetOffsetY()const;
    constexpr bool IsInsideGrid(int gx,int gy)const;
    constexpr bool HandleBattleSkillClick(int mouseX,int mouseY,Character* current);
};
'''

signatures = {
    'GetBattleSkillPanelRect': 'RECT GetBattleSkillPanelRect() const',
    'GetBattleSkillRect': 'RECT GetBattleSkillRect(int index) const',
    'GetBattleItemRect': 'RECT GetBattleItemRect(int index) const',
    'GetBattleCancelRect': 'RECT GetBattleCancelRect() const',
    'IsPointInRect': 'bool IsPointInRect(const RECT& rc, int x, int y) const',
    'ClearBattleSkillSelection': 'void ClearBattleSkillSelection()',
    'UpdateHoverGridByMouse': 'void UpdateHoverGridByMouse(int mouseX, int mouseY)',
    'UpdateSkillTargetCameraByMouse': 'void UpdateSkillTargetCameraByMouse(HWND hWnd)',
    'BackgroundMove': 'void BackgroundMove(int x, int y)',
    'FocusCameraOnWorldPos': 'void FocusCameraOnWorldPos(int wx, int wy)',
    'GetOffsetX': 'int GetOffsetX() const',
    'GetOffsetY': 'int GetOffsetY() const',
    'IsInsideGrid': 'bool IsInsideGrid(int gx, int gy) const',
    'HandleBattleSkillClick': 'bool HandleBattleSkillClick(int mouseX, int mouseY, Character* current)',
}
bodies = '\n'.join(
    'constexpr ' + signature.replace(name, 'Battle_Stage::' + name, 1) +
    '{' + production_body(name) + '}\n'
    for name, signature in signatures.items()
)

checks = r'''
constexpr bool PanelConsumesPointer(int width)
{
    Character caster,target;
    Battle_Stage base; base.rects.right=width;
    const RECT panel=base.GetBattleSkillPanelRect();
    const POINT points[]={{panel.left,panel.top},{panel.right,panel.top},
        {panel.left,panel.bottom},{panel.right,panel.bottom},
        {(panel.left+panel.right)/2,300},{panel.left,300},{panel.right,300}};
    for(const auto point:points)
    {
        Battle_Stage stage=base;
        stage.enemy=&target;
        stage.enemyX=(point.x-stage.GetOffsetX())/stage.TILE_SIZE;
        stage.enemyY=(point.y-stage.GetOffsetY())/stage.TILE_SIZE;
        stage.cursor=point;
        const float oldX=stage.camPosX,oldY=stage.camPosY;
        stage.UpdateSkillTargetCameraByMouse(0);
        if(stage.camPosX!=oldX || stage.camPosY!=oldY || stage.HoverGridX!=-1 || stage.HoverGridY!=-1)return false;
        if(!stage.HandleBattleSkillClick(point.x,point.y,&caster))return false;
        if(stage.casts!=0 || stage.enemyQueries!=0 || stage.SelectedBattleSkillID!=3301 ||
            !stage.bSelectingBattleSkillTarget || stage.HoverGridX!=-1 || stage.HoverGridY!=-1)return false;
    }
    return true;
}
constexpr bool SkillListAndBack()
{
    Character caster;
    Battle_Stage stage; stage.bSelectingBattleSkillTarget=false;stage.SelectedBattleSkillID=-1;
    RECT card=stage.GetBattleSkillRect(1);
    if(!stage.HandleBattleSkillClick(card.left+20,card.top+20,&caster))return false;
    if(stage.SelectedBattleSkillID!=3302 || !stage.bSelectingBattleSkillTarget ||
        stage.HoverGridX!=-1 || stage.HoverGridY!=-1 || stage.casts!=0)return false;
    RECT back=stage.GetBattleCancelRect();
    if(!stage.HandleBattleSkillClick(back.left+20,back.top+20,&caster))return false;
    if(stage.bSelectingBattleSkillTarget || stage.SelectedBattleSkillID!=-1 ||
        stage.BattleSkillIDs.size()!=3 || stage.HoverGridX!=-1 || stage.HoverGridY!=-1)return false;
    if(!stage.HandleBattleSkillClick(back.left+20,back.top+20,&caster))return false;
    return stage.controlMode==EBattleControlMode::WAIT_COMMAND && stage.BattleSkillIDs.empty() && stage.casts==0;
}
constexpr bool LowMPStillRejectsSelection()
{
    Character caster;caster.stat.CurMP=5;
    Battle_Stage stage;stage.bSelectingBattleSkillTarget=false;stage.SelectedBattleSkillID=-1;
    const RECT card=stage.GetBattleSkillRect(0);
    stage.HandleBattleSkillClick(card.left+20,card.top+20,&caster);
    return stage.SelectedBattleSkillID==-1 && !stage.bSelectingBattleSkillTarget &&
        stage.casts==0 && stage.notices==1 && caster.stat.CurMP==5;
}
constexpr bool WorldTargetStillUsesValidation(int mode)
{
    Character caster,target;
    Battle_Stage stage;stage.enemy=mode==1?nullptr:&target;stage.allowTarget=mode!=2;
    stage.HoverGridX=0;stage.HoverGridY=0;
    stage.HandleBattleSkillClick(424,336,&caster);
    return stage.enemyQueries==1 && stage.HoverGridX==15 && stage.HoverGridY==14 &&
        stage.casts==(mode==0?1:0) && (mode!=0 || stage.castTarget==&target);
}
constexpr bool KeyboardPanUsesCurrentPointer(int dx,int dy)
{
    Character caster,target;
    Battle_Stage stage;stage.enemy=&target;stage.enemyX=15+dx;stage.enemyY=14+dy;
    stage.BackgroundMove(dx,dy);
    if(stage.HoverGridX!=stage.enemyX || stage.HoverGridY!=stage.enemyY)return false;
    // No WM_MOUSEMOVE is delivered before the click.
    stage.HandleBattleSkillClick(stage.LastMouseX,stage.LastMouseY,&caster);
    return stage.casts==1 && stage.castTarget==&target &&
        stage.HoverGridX==stage.enemyX && stage.HoverGridY==stage.enemyY;
}
constexpr bool StationaryPointerRefreshesEveryUpdate()
{
    Battle_Stage stage;stage.HoverGridX=0;stage.HoverGridY=0;
    stage.UpdateSkillTargetCameraByMouse(0);
    if(stage.HoverGridX!=15 || stage.HoverGridY!=14)return false;
    stage.cursor={520,384}; // Pointer moved without a queued WM_MOUSEMOVE.
    stage.UpdateSkillTargetCameraByMouse(0);
    return stage.HoverGridX==(stage.cursor.x-stage.GetOffsetX())/48 &&
        stage.HoverGridY==(stage.cursor.y-stage.GetOffsetY())/48;
}
constexpr bool EdgePanAndHoverStayTogether(int edge)
{
    Battle_Stage stage;
    const POINT points[]={{8,300},{1275,300},{500,8},{500,715}};
    stage.cursor=points[edge];
    const float oldX=stage.camPosX,oldY=stage.camPosY;
    stage.UpdateSkillTargetCameraByMouse(0);
    if(edge==0 && !(stage.camPosX<oldX))return false;
    if(edge==1 && !(stage.camPosX>oldX))return false;
    if(edge==2 && !(stage.camPosY<oldY))return false;
    if(edge==3 && !(stage.camPosY>oldY))return false;
    return stage.HoverGridX==(stage.cursor.x-stage.GetOffsetX())/48 &&
        stage.HoverGridY==(stage.cursor.y-stage.GetOffsetY())/48;
}
constexpr bool PanelApproachDoesNotScroll()
{
    Battle_Stage stage;stage.cursor={870,300}; // Map strip preceding the panel, formerly an implicit edge.
    stage.UpdateSkillTargetCameraByMouse(0);
    return stage.camPosX==960 && stage.camPosY==720 && stage.HoverGridX==(870-stage.GetOffsetX())/48;
}
constexpr bool OutsideClientClearsAndCannotCast()
{
    Character caster,target;
    const POINT points[]={{-1,300},{1280,300},{500,-1},{500,720}};
    for(const auto point:points)
    {
        Battle_Stage stage;stage.cursor=point;stage.enemy=&target;
        stage.UpdateSkillTargetCameraByMouse(0);
        stage.HandleBattleSkillClick(point.x,point.y,&caster);
        if(stage.HoverGridX!=-1 || stage.HoverGridY!=-1 || stage.enemyQueries || stage.casts ||
            stage.camPosX!=960 || stage.camPosY!=720)return false;
    }
    return true;
}
constexpr bool CursorFailureClears(int failure)
{
    Battle_Stage stage;stage.cursorAvailable=failure!=0;stage.conversionAvailable=failure!=1;
    stage.UpdateSkillTargetCameraByMouse(0);
    stage.BackgroundMove(1,0);
    return stage.LastMouseX==-1 && stage.LastMouseY==-1 && stage.HoverGridX==-1 && stage.HoverGridY==-1;
}
constexpr bool OtherModesPreserveMovement(int mode)
{
    Battle_Stage stage;
    stage.controlMode=mode==0?EBattleControlMode::WAIT_COMMAND:
        mode==1?EBattleControlMode::SELECT_MOVE:EBattleControlMode::SELECT_SKILL;
    if(mode==2)stage.bSelectingBattleSkillTarget=false;
    stage.BackgroundMove(1,0);
    if(stage.camPosX!=992 || stage.camPosY!=720 || stage.HoverGridX!=15 || stage.HoverGridY!=14)return false;
    stage.cursor={8,300};stage.UpdateSkillTargetCameraByMouse(0);
    return stage.camPosX==992 && stage.camPosY==720 && stage.HoverGridX==15 && stage.HoverGridY==14;
}
constexpr bool WorldBoundsStillReject()
{
    Battle_Stage stage;
    stage.Loc.second={0,0};stage.UpdateHoverGridByMouse(10,10);
    if(stage.HoverGridX!=-1 || stage.HoverGridY!=-1)return false;
    stage.Loc.second={3000,3000};stage.UpdateHoverGridByMouse(500,300);
    return stage.HoverGridX==-1 && stage.HoverGridY==-1;
}
constexpr bool InvalidCallerCannotSelect()
{
    Battle_Stage stage;Character dead;dead.alive=false;
    return !stage.HandleBattleSkillClick(1000,210,nullptr) &&
        !stage.HandleBattleSkillClick(1000,210,&dead) && stage.casts==0 && stage.enemyQueries==0;
}
static_assert(PanelConsumesPointer(1024) && PanelConsumesPointer(1280) && PanelConsumesPointer(1600),
    "Opaque panel and all its borders must consume clicks/hover and must not auto-scroll");
static_assert(SkillListAndBack(),"Panel interception must preserve skill-list selection and both Back levels");
static_assert(LowMPStillRejectsSelection(),"Insufficient MP must still block selecting a skill");
static_assert(WorldTargetStillUsesValidation(0) && WorldTargetStillUsesValidation(1) && WorldTargetStillUsesValidation(2),
    "Visible map clicks must retain target lookup and execution validation");
static_assert(KeyboardPanUsesCurrentPointer(1,0) && KeyboardPanUsesCurrentPointer(-1,0) &&
    KeyboardPanUsesCurrentPointer(0,1) && KeyboardPanUsesCurrentPointer(0,-1),
    "All four keyboard camera directions must update hover before the next mouse event");
static_assert(StationaryPointerRefreshesEveryUpdate(),"Target updates must refresh hover even when no edge scroll occurs");
static_assert(EdgePanAndHoverStayTogether(0) && EdgePanAndHoverStayTogether(1) &&
    EdgePanAndHoverStayTogether(2) && EdgePanAndHoverStayTogether(3),
    "Visible client edges must keep scrolling and immediately update their target cell");
static_assert(PanelApproachDoesNotScroll(),"Approaching the skill panel is not an edge-scroll gesture");
static_assert(OutsideClientClearsAndCannotCast(),"Leaving the client must clear the target and consume out-of-client clicks");
static_assert(CursorFailureClears(0) && CursorFailureClears(1),"Missing cursor coordinates cannot leave a stale target");
static_assert(OtherModesPreserveMovement(0) && OtherModesPreserveMovement(1) && OtherModesPreserveMovement(2),
    "Non-targeting camera and movement preview behavior must stay unchanged");
static_assert(WorldBoundsStillReject(),"World coordinates outside the grid must still be invalid");
static_assert(InvalidCallerCannotSelect(),"Null/dead input callers must not start targeting or a cast");
'''

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path,
                    default=ROOT / 'tests/bin/skill-target-input/skill_target_input_compile.cpp')
args = parser.parse_args()
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(prefix + bodies + checks, encoding='utf-8')
print(f'Prepared {len(signatures)} actual production bodies and 13 static_assert groups: {args.output}')
print('No native process executed; compile this generated file as C++20 to evaluate the checks.')
