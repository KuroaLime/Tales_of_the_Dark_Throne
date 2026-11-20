// Actual production-object regression harness. No game window, stage Init,
// gameplay/audio loop, OS pointer movement, or saved-game access is performed.
#include <windows.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#define private public
#define protected public
#include "Map_Stage.h"
#include "BattleRewardManager.h"
#include "UMouseMark.h"
#include "resource.h"
#undef protected
#undef private
// Only the legacy map/game-data fixtures above use access remapping. The
// maintenance stage and its presentation objects compile with real access
// control, so tests cannot depend on their private implementation state.
#include "Maintenance_Stage.h"
#include "UIFrameTransition.h"

namespace {
int checks = 0, snapshots = 0;
void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); ++checks; }
bool Inside(const RECT& outer, const RECT& inner) { return inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right && inner.bottom <= outer.bottom; }
bool Overlap(const RECT& a, const RECT& b) { return a.left < b.right && a.right > b.left && a.top < b.bottom && a.bottom > b.top; }
POINT Center(const RECT& r) { return { (r.left + r.right) / 2, (r.top + r.bottom) / 2 }; }
int Click(Maintenance_Stage& stage, const RECT& rect)
{
    const auto p = Center(rect);
    return stage.Input(nullptr, WM_LBUTTONDOWN, 0, MAKELPARAM(p.x, p.y));
}
void Tab(Maintenance_Stage& stage, EMaintenanceTab tab)
{
    Require(Click(stage, stage.GetTabRect(tab)) == -2, "Tab click attempted a stage transition");
    Require(stage.GetActiveTab() == tab, "Actual maintenance tab click did not select requested tab");
    // Business-logic checks operate on the completed tab. Transition-specific
    // checks below deliberately use Input and Draw without this fixture helper.
    stage.AdvanceTabPresentation(.18);
    Require(!stage.IsTabTransitioning(), "Tab did not settle after its presentation duration");
}
int PartyIndex(int id)
{
    const auto& party = GameManager::Get().GetParty();
    for (size_t i = 0; i < party.size(); ++i) if (party[i]->GetCharacterID() == id) return static_cast<int>(i);
    return -1;
}
void RosterClick(Maintenance_Stage& stage, int id)
{
    Tab(stage, EMaintenanceTab::Party);
    const auto& roster = GameManager::Get().GetRoster();
    for (size_t i = 0; i < roster.size(); ++i) if (roster[i]->GetCharacterID() == id) { Click(stage, stage.GetRosterSlotRect(static_cast<int>(i))); return; }
    throw std::runtime_error("Unknown test roster ID");
}
int InventoryIndex(int id)
{
    const auto& inventory = GameManager::Get().GetInventory();
    for (size_t i = 0; i < inventory.size(); ++i) if (inventory[i].ItemID == id) return static_cast<int>(i);
    throw std::runtime_error("Missing actual inventory item");
}
struct Canvas {
    int width, height;
    HDC dc=nullptr, scratch=nullptr;
    HBITMAP bitmap=nullptr;
    HGDIOBJ previous=nullptr;
    std::uint32_t* pixels=nullptr;
    Canvas(int w, int h) : width(w), height(h) {
        BITMAPINFO info{};
        info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER); info.bmiHeader.biWidth=w; info.bmiHeader.biHeight=-h;
        info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
        dc=CreateCompatibleDC(nullptr); scratch=CreateCompatibleDC(nullptr); void* data=nullptr;
        bitmap=CreateDIBSection(nullptr,&info,DIB_RGB_COLORS,&data,nullptr,0);
        Require(dc&&scratch&&bitmap&&data,"Offscreen canvas failed");
        pixels=static_cast<std::uint32_t*>(data); previous=SelectObject(dc,bitmap);
    }
    ~Canvas(){SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(scratch);DeleteDC(dc);}
    void Save(const std::filesystem::path& out,const std::string& name){
        GdiFlush();
        for(size_t i=0;i<static_cast<size_t>(width)*height;++i)pixels[i]|=0xff000000u;
        BITMAPFILEHEADER file{};BITMAPINFOHEADER info{};
        info.biSize=sizeof(info);info.biWidth=width;info.biHeight=-height;info.biPlanes=1;info.biBitCount=32;
        info.biSizeImage=static_cast<DWORD>(width*height*4);
        file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(info);file.bfSize=file.bfOffBits+info.biSizeImage;
        std::ofstream stream(out/(name+"-"+std::to_string(width)+"x"+std::to_string(height)+".bmp"),std::ios::binary);
        stream.write(reinterpret_cast<const char*>(&file),sizeof(file));stream.write(reinterpret_cast<const char*>(&info),sizeof(info));
        stream.write(reinterpret_cast<const char*>(pixels),info.biSizeImage);Require(bool(stream),"Cannot write native snapshot");++snapshots;
    }
};
int CALLBACK CollectText(HDC,HANDLETABLE*,const ENHMETARECORD* record,int,LPARAM user)
{
    auto* text=reinterpret_cast<std::wstring*>(user);
    if(record->iType==EMR_EXTTEXTOUTW&&record->nSize>=sizeof(EMREXTTEXTOUTW)){
        const auto* out=reinterpret_cast<const EMREXTTEXTOUTW*>(record);
        const auto& value=out->emrtext;
        if(!(value.fOptions&ETO_GLYPH_INDEX)&&value.offString+static_cast<size_t>(value.nChars)*sizeof(wchar_t)<=record->nSize){
            text->append(reinterpret_cast<const wchar_t*>(reinterpret_cast<const BYTE*>(record)+value.offString),value.nChars);
            text->push_back(L' ');
        }
    }
    return 1;
}
std::wstring RenderText(Maintenance_Stage& stage, Canvas& canvas)
{
    HDC metafile=CreateEnhMetaFileW(canvas.dc,nullptr,nullptr,nullptr);
    Require(metafile!=nullptr,"Cannot create GDI text capture");
    stage.Draw(metafile,canvas.scratch);
    HENHMETAFILE recording=CloseEnhMetaFile(metafile);
    Require(recording!=nullptr,"Cannot close GDI text capture");
    std::wstring text;RECT client{0,0,canvas.width,canvas.height};
    Require(EnumEnhMetaFile(nullptr,recording,CollectText,&text,&client)!=0,"Cannot enumerate actual Draw text");
    DeleteEnhMetaFile(recording);
    Require(!text.empty(),"Actual Draw emitted no Unicode text");
    return text;
}
void Has(const std::wstring& text,const std::wstring& expected,const char* reason)
{
    if(text.find(expected)==std::wstring::npos){std::wcerr<<L"Missing rendered text: "<<expected<<L"\nActual: "<<text<<L'\n';Require(false,reason);}
    ++checks;
}
void Shot(Maintenance_Stage& stage,Canvas& canvas,const std::filesystem::path& out,const std::string& name)
{
    stage.Draw(canvas.dc,canvas.scratch);stage.DrawCursor(canvas.dc,canvas.scratch);canvas.Save(out,name);
}
class OffscreenMaintenanceStage : public Maintenance_Stage
{
public:
    void PrepareOffscreen(const RECT& client)
    {
        rects=client;
        hBitBackground=LoadBitmapW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(IDB_BITMAP3));
        Require(hBitBackground!=nullptr,"Maintenance overview background resource missing");
        GetObject(hBitBackground,sizeof(BITMAP),&BackgroundInfo);
    }
};
void PrepareMaintenance(OffscreenMaintenanceStage& stage,const RECT& client) { stage.PrepareOffscreen(client); }
void Geometry(Maintenance_Stage& stage,const RECT& client)
{
    const RECT panel=stage.GetMainPanelRect();Require(Inside(client,panel),"Maintenance panel escapes actual client");
    std::vector<RECT> rects;
    for(int i=0;i<15;++i){const RECT r=stage.GetRosterSlotRect(i);Require(Inside(panel,r),"Roster character unreachable outside panel");rects.push_back(r);}
    for(size_t i=0;i<rects.size();++i)for(size_t j=i+1;j<rects.size();++j)Require(!Overlap(rects[i],rects[j]),"Roster hit targets overlap");
    for(int i=0;i<10;++i){Require(Inside(panel,stage.GetInventorySlotRect(i)),"Inventory target escapes panel");Require(Inside(panel,stage.GetShopSlotRect(i)),"Shop target escapes panel");}
    for(int i=0;i<3;++i){Require(Inside(panel,stage.GetSkillSlotRect(i)),"Skill slot escapes panel");Require(Inside(panel,stage.GetLearnedSkillRect(i)),"Learned skill target escapes panel");Require(Inside(panel,stage.GetEquipmentSlotRect(i)),"Equipment target escapes panel");}
}
std::vector<std::uint32_t> DrawPixels(Maintenance_Stage& stage,Canvas& canvas)
{
    stage.Draw(canvas.dc,canvas.scratch);GdiFlush();
    return {canvas.pixels,canvas.pixels+static_cast<size_t>(canvas.width)*canvas.height};
}
size_t DifferentPixels(const std::vector<std::uint32_t>& a,const std::vector<std::uint32_t>& b,
    int width,const RECT& rect)
{
    size_t changed=0;
    for(int y=rect.top;y<rect.bottom;++y)for(int x=rect.left;x<rect.right;++x){
        const size_t i=static_cast<size_t>(y)*width+x;
        if((a[i]&0xffffffu)!=(b[i]&0xffffffu))++changed;
    }
    return changed;
}
void CheckUIFrameTransitionBoundaries()
{
    Canvas canvas(96,64);const RECT client{0,0,96,64},bounds{10,8,80,56},region{20,16,60,40};
    auto fill=[&](COLORREF color){HBRUSH brush=CreateSolidBrush(color);Require(brush!=nullptr,"Cannot fill compositor fixture");FillRect(canvas.dc,&client,brush);DeleteObject(brush);GdiFlush();};
    auto pixels=[&](){GdiFlush();return std::vector<std::uint32_t>(canvas.pixels,canvas.pixels+canvas.width*canvas.height);};
    UIFrameSnapshot presented;UIRegionTransition transition;
    Require(!presented.HasFrame()&&!transition.Begin(presented,bounds,.16)&&!transition.IsActive(),"Empty snapshot started a region transition");
    const COLORREF oldColor=RGB(30,60,100),newColor=RGB(160,120,40);
    fill(oldColor);Require(presented.Capture(canvas.dc,bounds)&&presented.HasFrame()&&presented.Matches(bounds),"Raster snapshot did not retain its screen-space bounds");
    Require(!transition.Begin(presented,bounds,0)&&!transition.Begin(presented,bounds,-.1)&&
        !transition.Begin(presented,bounds,std::numeric_limits<double>::quiet_NaN()),"Invalid transition duration became active");

    HDC metafile=CreateEnhMetaFileW(canvas.dc,nullptr,nullptr,nullptr);Require(metafile!=nullptr,"Cannot create compositor metafile fixture");
    const RECT differentBounds{0,0,70,48};
    Require(!presented.Capture(metafile,differentBounds)&&presented.HasFrame()&&presented.Matches(bounds),"Metafile capture replaced a valid raster snapshot");
    Require(transition.Begin(presented,bounds,.16)&&transition.IsActive()&&transition.Progress()==0,"Valid captured frame did not begin its transition");
    Require(!transition.CanComposite(metafile,bounds)&&transition.Composite(metafile,bounds,region)&&transition.Progress()==0,
        "Metafile composition read a raster source or changed the transition clock");
    HENHMETAFILE record=CloseEnhMetaFile(metafile);Require(record!=nullptr,"Cannot close compositor metafile fixture");DeleteEnhMetaFile(record);
    transition.Advance(0);transition.Advance(-.1);transition.Advance(std::numeric_limits<double>::quiet_NaN());
    Require(transition.IsActive()&&transition.Progress()==0,"Invalid elapsed time advanced the transition");
    transition.Advance(.08);Require(transition.Progress()>.49&&transition.Progress()<.51,"Region transition midpoint is incorrect");
    fill(newColor);const auto incoming=pixels();Require(transition.Composite(canvas.dc,bounds,region),"Region compositor rejected its matching framebuffer bounds");
    const auto mixture=pixels();const RECT outside{bounds.left,bounds.top,region.left,region.top};
    const COLORREF middle=GetPixel(canvas.dc,30,24);
    Require(middle!=oldColor&&middle!=newColor&&DifferentPixels(incoming,mixture,canvas.width,region)>100,
        "Region compositor did not produce a visible intermediate blend");
    Require(DifferentPixels(incoming,mixture,canvas.width,outside)==0,"Region compositor changed pixels outside its requested region");

    // Retarget from a captured mixture, then overwrite the presented frame.
    // The outgoing transition must own an independent copy of what was seen.
    Require(presented.Capture(canvas.dc,bounds)&&transition.Begin(presented,bounds,.16),"Cannot retarget from the last visible mixture");
    fill(RGB(100,10,180));Require(presented.Capture(canvas.dc,bounds),"Cannot update the incoming snapshot independently");
    Require(transition.Composite(canvas.dc,bounds,bounds),"Retargeted transition could not composite its saved mixture");
    Require(DifferentPixels(mixture,pixels(),canvas.width,bounds)==0,"Retargeting snapped to an overwritten or stale source image");
    Require(!transition.CanComposite(canvas.dc,differentBounds)&&!transition.Composite(canvas.dc,differentBounds,differentBounds)&&!transition.IsActive(),
        "Bounds mismatch reused an invalid snapshot instead of cancelling the transition");
    Require(transition.Progress()==1,"Cancelled transition did not return a settled progress");
    presented.Reset();Require(!presented.HasFrame()&&!presented.Matches(bounds),"Reset snapshot retained its old frame or bounds");
}
void CheckTabPresentation(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Party);
    Canvas canvas(client.right,client.bottom);game.SetGold(1000);
    const RECT panel=stage.GetMainPanelRect();
    const RECT body{panel.left+16,panel.top+150,panel.right-16,panel.bottom-8};
    const RECT header{0,0,client.right,body.top};
    std::array<RECT,6> tabBounds{};
    for(int i=0;i<6;++i)tabBounds[i]=stage.GetTabRect(static_cast<EMaintenanceTab>(i));

    // Exercise the real cached Draw path, including an unchanged header/tab bar.
    const auto party=DrawPixels(stage,canvas);
    Require(Click(stage,stage.GetTabRect(EMaintenanceTab::Shop))==-2&&stage.GetActiveTab()==EMaintenanceTab::Shop,
        "Mouse tab navigation failed with a captured source body");
    Require(stage.IsTabTransitioning()&&stage.GetTabTransitionProgress()==0,"Captured tab did not begin transitioning");
    const auto start=DrawPixels(stage,canvas);canvas.Save(output,"maintenance-tab-start");
    Require(DifferentPixels(party,start,canvas.width,body)==0,"Tab start snapped away from its previously displayed body");
    for(int i=0;i<3;++i)Click(stage,stage.GetShopSlotRect(0));
    Require(game.GetGold()==1000&&game.GetInventory().empty()&&!stage.IsShopFeedbackActive(),"Transition accepted a shop purchase");
    stage.AdvanceTabPresentation(.09);
    Require(stage.IsTabTransitioning()&&stage.GetTabTransitionProgress()>0&&stage.GetTabTransitionProgress()<1,
        "Tab midpoint was not an active intermediate presentation");
    const auto middle=DrawPixels(stage,canvas);canvas.Save(output,"maintenance-tab-middle");
    Click(stage,stage.GetShopSlotRect(0));
    Require(game.GetGold()==1000&&game.GetInventory().empty(),"Mid-transition purchase changed resources");
    const double sameProgress=stage.GetTabTransitionProgress();
    Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
    Require(stage.GetTabTransitionProgress()==sameProgress,"Clicking the active tab restarted its transition");
    stage.AdvanceTabPresentation(.09);
    Require(!stage.IsTabTransitioning()&&stage.GetTabTransitionProgress()==1,"Tab completion did not release its input guard");
    const auto complete=DrawPixels(stage,canvas);canvas.Save(output,"maintenance-tab-complete");
    Require(DifferentPixels(start,middle,canvas.width,body)>1000&&DifferentPixels(middle,complete,canvas.width,body)>1000,
        "Actual GDI body did not pass through a distinct intermediate image");
    Require(DifferentPixels(start,middle,canvas.width,header)==0&&DifferentPixels(middle,complete,canvas.width,header)==0,
        "Tab transition blended or moved the fixed header/tab bar");
    Click(stage,stage.GetShopSlotRect(0));
    Require(game.GetGold()==1000-ItemData::GetPrice(101)&&game.GetItemCount(101)==1,
        "Completed tab did not accept exactly one normal purchase");

    // Keyboard shortcuts stay responsive, including repeat of the active key.
    for(int i=0;i<6;++i){
        DrawPixels(stage,canvas);stage.Input(nullptr,WM_KEYDOWN,'1'+i,0);
        Require(static_cast<int>(stage.GetActiveTab())==i&&stage.IsTabTransitioning(),"Keyboard 1..6 failed to start its real tab presentation");
        stage.AdvanceTabPresentation(.06);const double progress=stage.GetTabTransitionProgress();
        stage.Input(nullptr,WM_KEYDOWN,'1'+i,0);
        Require(stage.GetTabTransitionProgress()==progress,"Repeated keyboard tab shortcut restarted presentation");
        stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);
        Require(!stage.IsTabTransitioning(),"Keyboard-selected tab did not complete");
    }

    // Rapid retargeting must start at the last visible mixture, not an older tab.
    Click(stage,stage.GetTabRect(EMaintenanceTab::Equipment));stage.AdvanceTabPresentation(.06);
    const auto mixture=DrawPixels(stage,canvas);
    Click(stage,stage.GetTabRect(EMaintenanceTab::Skill));
    Require(stage.GetActiveTab()==EMaintenanceTab::Skill&&stage.GetTabTransitionProgress()==0,"Rapid retarget ignored the newest tab");
    const auto retarget=DrawPixels(stage,canvas);canvas.Save(output,"maintenance-tab-retarget");
    Require(DifferentPixels(mixture,retarget,canvas.width,body)==0,"Rapid retarget jumped instead of preserving the visible body");
    stage.AdvanceTabPresentation(.06);DrawPixels(stage,canvas);
    Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
    Require(stage.GetActiveTab()==EMaintenanceTab::Shop&&stage.IsTabTransitioning(),"Second rapid tab selection was lost");
    const int goldBefore=game.GetGold(),countBefore=game.GetItemCount(101);
    Click(stage,stage.GetShopSlotRect(0));
    Require(game.GetGold()==goldBefore&&game.GetItemCount(101)==countBefore,"Rapid retarget allowed a hidden body action");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);Click(stage,stage.GetShopSlotRect(0));
    Require(game.GetGold()==goldBefore-ItemData::GetPrice(101)&&game.GetItemCount(101)==countBefore+1,
        "Rapid retarget did not restore normal shop behavior on completion");

    // Skill slot/character/card clicks must not mutate the incoming body early.
    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Skill));
    auto* target=game.GetParty()[stage.GetSelectedPartyIndex()];
    const int selectedParty=stage.GetSelectedPartyIndex(),selectedSkill=stage.GetSelectedSkillSlot();
    const int oldSkill=target->GetEquippedSkillID(2);const auto learned=target->GetLearnedSkillIDs();
    Click(stage,stage.GetPartySelectRect(1));Click(stage,stage.GetSkillSlotRect(2));Click(stage,stage.GetLearnedSkillRect(0));
    Require(stage.GetSelectedPartyIndex()==selectedParty&&stage.GetSelectedSkillSlot()==selectedSkill&&target->GetEquippedSkillID(2)==oldSkill,
        "Transition accepted a party target or skill assignment");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);
    Click(stage,stage.GetSkillSlotRect(2));Click(stage,stage.GetLearnedSkillRect(0));
    Require(target->GetEquippedSkillID(2)==learned[0],"Completed skill tab did not equip the requested skill");

    // Inventory and its automatic Equipment transition share the same guard.
    game.AddItem(201,1);auto stats=target->GetStat();stats.CurHP=20;target->SetStat(stats);
    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Inventory));
    const int potions=game.GetItemCount(101);
    Click(stage,stage.GetPartySelectRect(1));Click(stage,stage.GetInventorySlotRect(InventoryIndex(201)));
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(101)));
    Require(stage.GetActiveTab()==EMaintenanceTab::Inventory&&stage.GetSelectedPartyIndex()==selectedParty&&
        target->GetEquipment().WeaponID==-1&&game.GetItemCount(201)==1&&game.GetItemCount(101)==potions&&target->GetStat().CurHP==20,
        "Transition accepted an inventory equip, potion, or target change");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(101)));
    Require(target->GetStat().CurHP==70&&game.GetItemCount(101)==potions-1,"Completed inventory tab did not consume/heal normally");
    DrawPixels(stage,canvas);Click(stage,stage.GetInventorySlotRect(InventoryIndex(201)));
    Require(stage.GetActiveTab()==EMaintenanceTab::Equipment&&stage.IsTabTransitioning()&&target->GetEquipment().WeaponID==201&&game.GetItemCount(201)==0,
        "Inventory equip did not preserve its automatic Equipment tab transition");
    Click(stage,stage.GetEquipmentSlotRect(0));
    Require(target->GetEquipment().WeaponID==201&&game.GetItemCount(201)==0,"Automatic Equipment transition accepted premature unequip");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);Click(stage,stage.GetEquipmentSlotRect(0));
    Require(target->GetEquipment().WeaponID==-1&&game.GetItemCount(201)==1,"Completed Equipment tab did not restore normal unequip");

    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Party));
    const size_t partyCount=game.GetParty().size();Click(stage,stage.GetRosterSlotRect(0));
    Require(game.GetParty().size()==partyCount&&PartyIndex(1)>=0,"Transition changed the active party");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);Click(stage,stage.GetRosterSlotRect(0));
    Require(game.GetParty().size()==partyCount-1&&PartyIndex(1)<0,"Completed Party tab did not restore roster actions");
    for(int i=0;i<6;++i){const RECT current=stage.GetTabRect(static_cast<EMaintenanceTab>(i));
        Require(EqualRect(&tabBounds[i],&current)!=FALSE,"Tab presentation changed tab hit geometry");}
    Geometry(stage,client);
    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Inventory));
    Require(stage.IsTabTransitioning()&&stage.Input(nullptr,WM_KEYDOWN,VK_ESCAPE,0)==MAP_STAGE,
        "Tab transition trapped the original Escape return route");
    Require(Click(stage,stage.GetContinueRect())==MAP_STAGE,"Tab transition trapped the original map button route");
}
void CheckCharacterPresentation(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    const auto party=game.GetParty();Require(party.size()==5,"Character presentation fixture requires the default five members");
    const std::array<int,5> weapons{201,202,201,203,203};
    for(size_t i=0;i<party.size();++i){game.AddItem(weapons[i],1);Require(game.EquipItem(party[i],weapons[i]),"Cannot prepare real equipped-character fixture");}
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Skill);
    Canvas canvas(client.right,client.bottom);
    Click(stage,stage.GetSkillSlotRect(2));Require(stage.GetSelectedSkillSlot()==2,"Cannot prepare selected skill slot through actual input");
    Tab(stage,EMaintenanceTab::Equipment);
    const RECT panel=stage.GetMainPanelRect();
    const RECT details{panel.left+16,panel.top+202,panel.right-16,panel.bottom-8};
    const RECT body{panel.left+16,panel.top+150,panel.right-16,panel.bottom-8};
    const RECT fixed{0,0,client.right,details.top};
    std::array<RECT,5> selectorBounds{};
    for(int i=0;i<5;++i)selectorBounds[i]=stage.GetPartySelectRect(i);

    // Every party member can be selected, but an outgoing equipment card must
    // never unequip either the old or incoming member while it is blended.
    for(int selected:{1,2,3,4,0}){
        const auto old=DrawPixels(stage,canvas);
        Click(stage,stage.GetPartySelectRect(selected));
        Require(stage.GetSelectedPartyIndex()==selected&&stage.GetSelectedSkillSlot()==2&&stage.IsCharacterTransitioning()&&!stage.IsTabTransitioning(),
            "Equipment selection did not update immediately while retaining its skill slot");
        const auto start=DrawPixels(stage,canvas);
        Require(DifferentPixels(old,start,canvas.width,details)==0,"Character fade replaced its old detail image before progressing");
        Require(DifferentPixels(old,start,canvas.width,selectorBounds[selected])>10,"Selected character highlight was not immediate");
        for(int repeat=0;repeat<3;++repeat)Click(stage,stage.GetEquipmentSlotRect(0));
        for(int i=0;i<5;++i)Require(party[i]->GetEquipment().WeaponID==weapons[i],"Character fade unequipped an old or incoming member");
        stage.AdvanceTabPresentation(.08);const double progress=stage.GetCharacterTransitionProgress();
        Require(stage.IsCharacterTransitioning()&&progress>0&&progress<1,"Character midpoint is not an intermediate presentation");
        const auto middle=DrawPixels(stage,canvas);
        if(selected==1)canvas.Save(output,"maintenance-character-equipment-middle");
        Click(stage,stage.GetPartySelectRect(selected));
        Require(stage.GetCharacterTransitionProgress()==progress,"Selecting the same character restarted its fade");
        stage.AdvanceTabPresentation(.08);
        Require(!stage.IsCharacterTransitioning()&&stage.GetCharacterTransitionProgress()==1,"Character completion kept detail actions blocked");
        const auto complete=DrawPixels(stage,canvas);
        canvas.Save(output,"equipment-character-"+std::to_string(selected));
        Require(DifferentPixels(start,middle,canvas.width,fixed)==0&&DifferentPixels(middle,complete,canvas.width,fixed)==0,
            "Character fade blended the selector, tabs, or header");
        Require(DifferentPixels(start,middle,canvas.width,details)>100&&DifferentPixels(middle,complete,canvas.width,details)>100,
            "Equipment details did not visibly pass through an intermediate frame");
        if(selected==1)canvas.Save(output,"maintenance-character-equipment-complete");
        const auto text=RenderText(stage,canvas);Has(text,L"Current Stats","Selected equipment detail lost its stat heading");
        Has(text,ItemData::GetName(weapons[selected]),"Selected equipment detail lost its equipped item name");
        const int count=game.GetItemCount(weapons[selected]);Click(stage,stage.GetEquipmentSlotRect(0));
        Require(party[selected]->GetEquipment().WeaponID==-1&&game.GetItemCount(weapons[selected])==count+1,
            "Completed equipment selection did not unequip the exact selected target");
        for(int i=0;i<5;++i)if(i!=selected)Require(party[i]->GetEquipment().WeaponID==weapons[i],"Selected-target unequip changed another member");
        Require(game.EquipItem(party[selected],weapons[selected]),"Cannot restore equipment fixture for the next selection");
    }

    Tab(stage,EMaintenanceTab::Skill);
    for(int selected:{1,2,3,4,0}){
        DrawPixels(stage,canvas);
        std::vector<std::vector<int>> previous;for(auto* member:party)previous.push_back(member->GetEquippedSkillIDs());
        Click(stage,stage.GetPartySelectRect(selected));
        Require(stage.GetSelectedPartyIndex()==selected&&stage.GetSelectedSkillSlot()==2&&stage.IsCharacterTransitioning(),
            "Skill character selection lost its logical target or retained slot");
        const auto start=DrawPixels(stage,canvas);
        Click(stage,stage.GetSkillSlotRect(0));Click(stage,stage.GetLearnedSkillRect(0));
        Require(stage.GetSelectedSkillSlot()==2,"Character fade accepted a stale skill-slot click");
        for(int i=0;i<5;++i)Require(party[i]->GetEquippedSkillIDs()==previous[i],"Character fade assigned a skill to a hidden target");
        stage.AdvanceTabPresentation(.08);const auto middle=DrawPixels(stage,canvas);
        if(selected==1)canvas.Save(output,"maintenance-character-skill-middle");
        stage.AdvanceTabPresentation(.08);const auto complete=DrawPixels(stage,canvas);
        canvas.Save(output,"skill-character-"+std::to_string(selected));
        Require(!stage.IsCharacterTransitioning()&&DifferentPixels(start,middle,canvas.width,fixed)==0&&DifferentPixels(middle,complete,canvas.width,fixed)==0,
            "Skill selection did not complete with a stable selector/header");
        if(selected==1)canvas.Save(output,"maintenance-character-skill-complete");
        const auto learned=party[selected]->GetLearnedSkillIDs();
        const auto text=RenderText(stage,canvas);
        Has(text,party[selected]->GetName()+L" / "+party[selected]->GetElementName(),"Selected skill detail has the wrong identity");
        for(int id:learned)Has(text,SkillData::GetName(id),"Selected character's learned skill disappeared");
        Click(stage,stage.GetLearnedSkillRect(0));
        Require(party[selected]->GetEquippedSkillID(2)==learned[0]&&party[selected]->GetEquippedSkillID(0)==previous[selected][0]&&
            party[selected]->GetEquippedSkillID(1)==previous[selected][1],"Completed selection equipped the wrong character or slot");
        for(int i=0;i<5;++i)if(i!=selected)Require(party[i]->GetEquippedSkillIDs()==previous[i],"Selected-target skill assignment modified another member");
    }

    // A second character and then another tab both retarget from the last
    // displayed mixture. Only the relevant detail region blends for selection.
    for(const auto tab:{EMaintenanceTab::Equipment,EMaintenanceTab::Skill}){
        Tab(stage,tab);DrawPixels(stage,canvas);
        Click(stage,stage.GetPartySelectRect(1));stage.AdvanceTabPresentation(.05);
        const auto mixture=DrawPixels(stage,canvas);
        Click(stage,stage.GetPartySelectRect(2));
        Require(stage.GetSelectedPartyIndex()==2&&stage.IsCharacterTransitioning()&&stage.GetCharacterTransitionProgress()==0,
            "Rapid character selection ignored the latest target");
        const auto retarget=DrawPixels(stage,canvas);
        Require(DifferentPixels(mixture,retarget,canvas.width,details)==0,"Rapid character retarget jumped to an older detail frame");
        stage.AdvanceTabPresentation(.05);DrawPixels(stage,canvas);
        Click(stage,stage.GetPartySelectRect(3));stage.AdvanceTabPresentation(.05);
        const auto lastVisible=DrawPixels(stage,canvas);
        Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
        Require(stage.IsTabTransitioning()&&!stage.IsCharacterTransitioning(),"Character-to-tab retarget left competing transitions active");
        const auto tabStart=DrawPixels(stage,canvas);
        Require(DifferentPixels(lastVisible,tabStart,canvas.width,body)==0,"Character-to-tab transition did not preserve the visible mixed frame");
        Click(stage,stage.GetPartySelectRect(0));
        Require(stage.GetSelectedPartyIndex()==3,"Tab transition allowed its outgoing selector to change character");
        stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);
        Require(!stage.IsTabTransitioning()&&!stage.IsCharacterTransitioning()&&stage.GetActiveTab()==EMaintenanceTab::Shop,
            "Character-to-tab retarget did not reach the latest tab");
    }

    Tab(stage,EMaintenanceTab::Inventory);game.AddItem(101,1);DrawPixels(stage,canvas);
    for(int i=0;i<5;++i){Click(stage,stage.GetPartySelectRect(i));
        Require(stage.GetSelectedPartyIndex()==i&&!stage.IsCharacterTransitioning()&&!stage.IsTabTransitioning(),"Inventory character selection stopped being immediate");}
    auto stats=party[1]->GetStat();stats.CurHP=20;party[1]->SetStat(stats);
    Click(stage,stage.GetPartySelectRect(1));Click(stage,stage.GetInventorySlotRect(InventoryIndex(101)));
    Require(party[1]->GetStat().CurHP==70&&game.GetItemCount(101)==0,"Immediate inventory target did not receive its potion");
    for(const auto tab:{EMaintenanceTab::Equipment,EMaintenanceTab::Skill}){
        Tab(stage,tab);DrawPixels(stage,canvas);
        Require(stage.GetSelectedPartyIndex()==1&&stage.GetSelectedSkillSlot()==2,"Returning from Inventory lost the shared target or skill slot");
    }
    for(int i=0;i<5;++i){const RECT current=stage.GetPartySelectRect(i);
        Require(EqualRect(&selectorBounds[i],&current)!=FALSE,"Character presentation changed the selector hit geometry");}
    const std::string size=std::to_string(client.right)+"x"+std::to_string(client.bottom);
    std::ofstream metadata(output/("character-stills-"+size+".json"));
    metadata<<"{\n  \"scope\": \"Actual GDI completed-state snapshots already drawn by the functional regression check; no animation sequence or game recording. Captured before each follow-up equip/skill action.\",\n"
        <<"  \"width\": "<<client.right<<", \"height\": "<<client.bottom<<",\n"
        <<"  \"panelRect\": ["<<panel.left<<","<<panel.top<<","<<panel.right<<","<<panel.bottom<<"],\n"
        <<"  \"bodyRect\": ["<<body.left<<","<<body.top<<","<<body.right<<","<<body.bottom<<"],\n"
        <<"  \"detailRect\": ["<<details.left<<","<<details.top<<","<<details.right<<","<<details.bottom<<"],\n"
        <<"  \"fixtureGold\": 0, \"selectedSkillSlotIndex\": 2,\n"
        <<"  \"fixtureNote\": \"Default five-member party, fixture weapons equipped through GameManager, Slot 3 selected. Not player saved-game state.\",\n  \"actors\": [\n";
    for(int i=0;i<5;++i){
        metadata<<"    {\"partyIndex\":"<<i<<",\"characterId\":"<<party[i]->GetCharacterID()<<",\"equipmentWeaponId\":"<<weapons[i]
            <<",\"equipmentFile\":\"equipment-character-"<<i<<"-"<<size<<".bmp\",\"skillFile\":\"skill-character-"<<i<<"-"<<size<<".bmp\"}"<<(i==4?"\n":",\n");
    }
    metadata<<"  ]\n}\n";
    Require(bool(metadata),"Cannot save character visual-QA metadata");
}
void CheckActionFeedback(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Equipment);
    Canvas canvas(client.right,client.bottom);auto* target=game.GetParty()[0];
    auto base=target->GetStat();base.CurHP=37;base.CurMP=7;target->SetStat(base);
    game.AddItem(302,1);game.SetGold(275);
    const RECT panel=stage.GetMainPanelRect();
    const RECT fixed{0,0,client.right,panel.top+202};
    const RECT details{panel.left+16,panel.top+202,panel.right-16,panel.bottom-8};
    const std::string size=std::to_string(client.right)+"x"+std::to_string(client.bottom);
    std::ofstream metadata(output/("action-feedback-stills-"+size+".json"));
    metadata<<"{\n  \"scope\": \"Actual finite functional-check GDI checkpoints, not a continuous native recording or video renderer. No saved-game state.\",\n"
        <<"  \"width\": "<<client.right<<", \"height\": "<<client.bottom<<",\n"
        <<"  \"panelRect\": ["<<panel.left<<","<<panel.top<<","<<panel.right<<","<<panel.bottom<<"],\n"
        <<"  \"bodyRect\": ["<<panel.left+16<<","<<panel.top+150<<","<<panel.right-16<<","<<panel.bottom-8<<"],\n"
        <<"  \"detailRect\": ["<<details.left<<","<<details.top<<","<<details.right<<","<<details.bottom<<"],\n"
        <<"  \"fixture\": \"Default Fire Warrior, GOLD 275, current HP 37 and MP 7. Iron Armor equipped and unequipped through actual inventory/equipment input. Existing current/max HP bonus rules preserved. Same actor then changes skill slots and assignments.\",\n"
        <<"  \"snapshots\": [\n";
    bool firstSnapshot=true;
    auto checkpoint=[&](const char* label,const char* event,double age){
        const auto pixels=DrawPixels(stage,canvas);canvas.Save(output,std::string("action-feedback-")+label);
        const auto stat=target->GetStat();const auto equip=target->GetEquipment();
        if(!firstSnapshot)metadata<<",\n";firstSnapshot=false;
        metadata<<"    {\"label\":\""<<label<<"\",\"file\":\"action-feedback-"<<label<<"-"<<size<<".bmp\",\"event\":\""<<event
            <<"\",\"presentationAgeSeconds\":"<<age<<",\"tab\":"<<static_cast<int>(stage.GetActiveTab())<<",\"characterId\":"<<target->GetCharacterID()
            <<",\"selectedSkillSlot\":"<<stage.GetSelectedSkillSlot()<<",\"gold\":"<<game.GetGold()<<",\"stats\":{\"MaxHP\":"<<stat.MaxHP<<",\"CurHP\":"<<stat.CurHP
            <<",\"MaxMP\":"<<stat.MaxMP<<",\"CurMP\":"<<stat.CurMP<<",\"STR\":"<<stat.STR<<",\"INT\":"<<stat.INT<<",\"DEF\":"<<stat.DEF<<",\"SPD\":"<<stat.SPD
            <<"},\"equipment\":["<<equip.WeaponID<<","<<equip.ArmorID<<","<<equip.AccessoryID<<"],\"equippedSkillIds\":["<<target->GetEquippedSkillID(0)<<","<<target->GetEquippedSkillID(1)<<","<<target->GetEquippedSkillID(2)
            <<"],\"equipmentFeedbackActive\":"<<(stage.IsEquipmentFeedbackActive()?"true":"false")<<",\"skillFeedbackActive\":"<<(stage.IsSkillFeedbackTransitioning()?"true":"false")<<"}";
        return pixels;
    };
    auto sameStats=[](const CharacterStat& a,const CharacterStat& b){
        return a.MaxHP==b.MaxHP&&a.CurHP==b.CurHP&&a.MaxMP==b.MaxMP&&a.CurMP==b.CurMP&&a.STR==b.STR&&a.INT==b.INT&&a.DEF==b.DEF&&a.SPD==b.SPD;
    };

    // Preserve the real inventory -> Equipment route and immediate data, while
    // checking readable mixed positive/negative feedback from one real item.
    DrawPixels(stage,canvas);Tab(stage,EMaintenanceTab::Inventory);
    checkpoint("inventory-before","baseline",0);
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(302)));
    Require(stage.GetActiveTab()==EMaintenanceTab::Equipment&&stage.IsTabTransitioning(),"Feedback equip lost the existing automatic Equipment tab route");
    const auto armored=target->GetStat();
    Require(target->GetEquipment().ArmorID==302&&game.GetItemCount(302)==0&&game.GetGold()==275,"Armor feedback changed inventory accounting or gold");
    Require(armored.MaxHP==base.MaxHP+20&&armored.CurHP==base.CurHP+20&&armored.DEF==base.DEF+6&&armored.SPD==base.SPD-1&&armored.CurMP==base.CurMP,
        "Armor feedback changed original current/max HP or mixed stat-bonus rules");
    Require(stage.IsEquipmentFeedbackActive()&&stage.GetEquipmentStatDelta(0)==20&&stage.GetEquipmentStatDelta(4)==6&&stage.GetEquipmentStatDelta(5)==-1,
        "Armor feedback did not report real positive and negative stat deltas");
    Require(stage.GetEquipmentStatDelta(1)==0&&stage.GetEquipmentStatDelta(2)==0&&stage.GetEquipmentStatDelta(3)==0,"Unchanged armor stats got invented deltas");
    stage.AdvanceTabPresentation(.18);const auto equippedStart=checkpoint("equip-start","equip",.18);
    auto text=RenderText(stage,canvas);
    Has(text,L"+20","Armor HP gain not visible");Has(text,L"+6","Armor DEF gain not visible");Has(text,L"-1","Armor SPD cost not visible");
    Has(text,std::to_wstring(armored.CurHP)+L" / "+std::to_wstring(armored.MaxHP),"Feedback obscured actual current/max HP");
    stage.AdvanceTabPresentation(.92);checkpoint("equip-middle","equip",1.10);
    stage.AdvanceTabPresentation(.50);const auto equippedEnd=checkpoint("equip-expired","equip",1.60);
    Require(!stage.IsEquipmentFeedbackActive()&&sameStats(target->GetStat(),armored),"Equipment highlight did not expire independently of real stats");
    Require(DifferentPixels(equippedStart,equippedEnd,canvas.width,details)>30,"Equipment feedback never changed visible detail pixels");
    Require(DifferentPixels(equippedStart,equippedEnd,canvas.width,fixed)==0,"Equipment feedback affected portrait selector/header layout");

    Click(stage,stage.GetEquipmentSlotRect(1));
    Require(target->GetEquipment().ArmorID==-1&&game.GetItemCount(302)==1&&sameStats(target->GetStat(),base),"Unequip feedback changed original stats or inventory return");
    Require(stage.IsEquipmentFeedbackActive()&&stage.GetEquipmentStatDelta(0)==-20&&stage.GetEquipmentStatDelta(4)==-6&&stage.GetEquipmentStatDelta(5)==1,
        "Unequip feedback did not reverse the actual mixed bonuses");
    checkpoint("unequip-start","unequip",0);
    stage.AdvanceTabPresentation(.12);checkpoint("unequip-middle","unequip",.12);
    text=RenderText(stage,canvas);Has(text,L"-20","Unequip HP loss missing");Has(text,L"-6","Unequip DEF loss missing");Has(text,L"+1","Unequip SPD gain missing");
    stage.AdvanceTabPresentation(1.48);checkpoint("unequip-expired","unequip",1.60);
    Require(!stage.IsEquipmentFeedbackActive(),"Unequip feedback did not complete");
    Click(stage,stage.GetEquipmentSlotRect(1));
    Require(!stage.IsEquipmentFeedbackActive()&&game.GetItemCount(302)==1&&sameStats(target->GetStat(),base),"Empty unequip invented a stat change or duplicated an item");

    // Selection and assignment use immediate logical targets even while the
    // local slot artwork is blending. The shared tab/character guards remain.
    Tab(stage,EMaintenanceTab::Skill);const auto learned=target->GetLearnedSkillIDs();
    Require(learned.size()==3,"Skill feedback requires the normal three learned skills");
    const auto skillBefore=target->GetEquippedSkillIDs();checkpoint("skill-before","baseline",0);
    Click(stage,stage.GetSkillSlotRect(1));
    Require(stage.GetSelectedSkillSlot()==1&&stage.IsSkillFeedbackTransitioning(),"Slot selection was delayed by its visual transition");
    const auto selectStart=checkpoint("select-start","select",0);
    stage.AdvanceTabPresentation(.07);const auto selectMiddle=checkpoint("select-middle","select",.07);
    const double selectedProgress=stage.GetSkillFeedbackProgress();Click(stage,stage.GetSkillSlotRect(1));
    Require(stage.GetSkillFeedbackProgress()==selectedProgress,"Selecting the same skill slot restarted its transition");
    stage.AdvanceTabPresentation(.07);const auto selectEnd=checkpoint("select-complete","select",.14);
    Require(!stage.IsSkillFeedbackTransitioning()&&target->GetEquippedSkillIDs()==skillBefore,"Slot selection modified equipped skills");
    Require(DifferentPixels(selectStart,selectMiddle,canvas.width,details)>20&&DifferentPixels(selectMiddle,selectEnd,canvas.width,details)>20,"Skill selected-slot highlight did not blend through a visible midpoint");
    Require(DifferentPixels(selectStart,selectEnd,canvas.width,fixed)==0,"Skill slot selection changed the fixed selector/header");

    const int mp=target->GetStat().CurMP;
    Click(stage,stage.GetLearnedSkillRect(2));
    Require(target->GetEquippedSkillID(1)==learned[2]&&stage.GetSelectedSkillSlot()==1&&stage.IsSkillFeedbackTransitioning(),"Skill replacement did not apply to the immediate selected slot");
    Require(target->GetEquippedSkillID(0)==skillBefore[0]&&target->GetEquippedSkillID(2)==skillBefore[2]&&target->GetStat().CurMP==mp&&game.GetGold()==275,
        "Skill replacement changed another slot or spent combat/gold resources");
    checkpoint("replace-start","replace",0);stage.AdvanceTabPresentation(.08);checkpoint("replace-middle","replace",.08);
    const double replacementProgress=stage.GetSkillFeedbackProgress();Click(stage,stage.GetLearnedSkillRect(2));
    Require(stage.GetSkillFeedbackProgress()==replacementProgress,"Assigning the already-equipped skill restarted its visual transition");
    Has(RenderText(stage,canvas),L"Equipped "+SkillData::GetName(learned[2])+L" to slot 2.","No-op assignment lost its original user feedback message");
    stage.AdvanceTabPresentation(.08);checkpoint("replace-complete","replace",.16);
    Require(!stage.IsSkillFeedbackTransitioning(),"Skill content fade did not finish within its duration");
    text=RenderText(stage,canvas);Has(text,L"Slot 2: "+SkillData::GetName(learned[2]),"Completed skill replacement displayed stale slot contents");
    stage.AdvanceTabPresentation(.44);checkpoint("replace-expired","replace",.60);

    Click(stage,stage.GetLearnedSkillRect(0));stage.AdvanceTabPresentation(.05);const auto visibleMixture=DrawPixels(stage,canvas);
    Click(stage,stage.GetLearnedSkillRect(1));
    Require(target->GetEquippedSkillID(1)==learned[1]&&stage.GetSkillFeedbackProgress()==0,"Rapid skill replacement ignored its final data target");
    const auto rapidStart=checkpoint("rapid-start","rapid",0);
    // Inspect the slot/card content region independently of its status footer.
    const RECT skills{stage.GetSkillSlotRect(0).left,stage.GetSkillSlotRect(0).top,stage.GetSkillSlotRect(2).right,stage.GetLearnedSkillRect(2).bottom};
    Require(DifferentPixels(visibleMixture,rapidStart,canvas.width,skills)==0,"Rapid skill retarget snapped away from the currently visible mixture");
    stage.AdvanceTabPresentation(.08);checkpoint("rapid-middle","rapid",.08);
    stage.AdvanceTabPresentation(.08);const auto rapidComplete=checkpoint("rapid-complete","rapid",.16);
    Require(!stage.IsSkillFeedbackTransitioning()&&target->GetEquippedSkillID(1)==learned[1],"Rapid replacement did not settle on the last chosen skill");
    stage.AdvanceTabPresentation(.44);const auto rapidExpired=checkpoint("rapid-expired","rapid",.60);
    Require(DifferentPixels(rapidComplete,rapidExpired,canvas.width,skills)>20,"Skill success pulse failed to return to its normal selected appearance");
    Click(stage,stage.GetSkillSlotRect(2));Click(stage,stage.GetLearnedSkillRect(0));
    Require(stage.GetSelectedSkillSlot()==2&&target->GetEquippedSkillID(2)==learned[0]&&target->GetEquippedSkillID(1)==learned[1],"New selected slot was not actionable while its highlight was transitioning");
    Click(stage,stage.GetPartySelectRect(1));
    Require(!stage.IsEquipmentFeedbackActive()&&!stage.IsSkillFeedbackTransitioning()&&stage.GetSelectedSkillSlot()==2,"Character switch leaked old local action feedback or reset the slot");
    stage.AdvanceTabPresentation(.16);DrawPixels(stage,canvas);Click(stage,stage.GetPartySelectRect(0));stage.AdvanceTabPresentation(.16);DrawPixels(stage,canvas);

    // Preserve successful no-op, failed, and back-to-back equipment semantics.
    Tab(stage,EMaintenanceTab::Inventory);Click(stage,stage.GetInventorySlotRect(InventoryIndex(302)));stage.AdvanceTabPresentation(1.8);DrawPixels(stage,canvas);
    game.AddItem(302,1);Tab(stage,EMaintenanceTab::Inventory);DrawPixels(stage,canvas);
    const auto beforeSame=target->GetStat();const int sameCount=game.GetItemCount(302);
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(302)));
    Require(!stage.IsEquipmentFeedbackActive()&&game.GetItemCount(302)==sameCount&&sameStats(target->GetStat(),beforeSame),"Identical armor re-equip invented delta feedback or changed accounting");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);
    Click(stage,stage.GetEquipmentSlotRect(1));stage.AdvanceTabPresentation(.10);DrawPixels(stage,canvas);
    Click(stage,stage.GetPartySelectRect(1));
    Require(!stage.IsEquipmentFeedbackActive(),"Character change kept another member's stat deltas");
    stage.AdvanceTabPresentation(.16);DrawPixels(stage,canvas);Click(stage,stage.GetPartySelectRect(0));stage.AdvanceTabPresentation(.16);DrawPixels(stage,canvas);
    game.AddItem(201,1);game.AddItem(402,1);game.AddItem(203,1);
    for(int id:{201,302,402}){Tab(stage,EMaintenanceTab::Inventory);Click(stage,stage.GetInventorySlotRect(InventoryIndex(id)));stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);}
    stage.AdvanceTabPresentation(1.6);
    Click(stage,stage.GetEquipmentSlotRect(0));Click(stage,stage.GetEquipmentSlotRect(1));Click(stage,stage.GetEquipmentSlotRect(2));
    Require(target->GetEquipment().WeaponID==-1&&target->GetEquipment().ArmorID==-1&&target->GetEquipment().AccessoryID==-1&&sameStats(target->GetStat(),base),"Equipment feedback blocked consecutive unequips or failed to restore actual stats");
    Require(game.GetItemCount(201)==1&&game.GetItemCount(302)==2&&game.GetItemCount(402)==1,"Consecutive unequip lost or duplicated inventory items");
    Require(stage.GetEquipmentStatDelta(0)==-20&&stage.GetEquipmentStatDelta(2)==-5&&stage.GetEquipmentStatDelta(4)==-6&&stage.GetEquipmentStatDelta(5)==-2,
        "Consecutive unequip deltas did not aggregate actual before/after stats");
    Tab(stage,EMaintenanceTab::Inventory);Require(!stage.IsEquipmentFeedbackActive(),"Tab change retained stale stat deltas");
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(203)));stage.AdvanceTabPresentation(.18);
    Require(target->GetStat().MaxMP==base.MaxMP+10&&target->GetStat().CurMP==base.CurMP+10&&stage.GetEquipmentStatDelta(1)==10&&stage.GetEquipmentStatDelta(3)==6,
        "Staff feedback lost current/max MP or INT bonus semantics");
    stage.AdvanceTabPresentation(1.6);Click(stage,stage.GetEquipmentSlotRect(0));
    Require(sameStats(target->GetStat(),base)&&stage.GetEquipmentStatDelta(1)==-10,"Staff unequip feedback did not restore current/max MP");
    stage.AdvanceTabPresentation(1.6);DrawPixels(stage,canvas);
    Require(!game.EquipItem(target,99999)&&!game.UnequipItem(target,EEquipPart::Weapon)&&!stage.IsEquipmentFeedbackActive(),"Invalid/empty equipment actions produced fake presentation feedback");
    Tab(stage,EMaintenanceTab::Skill);DrawPixels(stage,canvas);Click(stage,stage.GetLearnedSkillRect(1));
    Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
    Require(!stage.IsSkillFeedbackTransitioning()&&stage.GetSelectedSkillSlot()==2,"Tab change leaked the outgoing skill action animation or changed selection");

    // Imported or otherwise externally populated learned lists can contain a
    // skill that the existing class/element rules reject at assignment time.
    // Such clicks must not masquerade as a successful replacement animation.
    Tab(stage,EMaintenanceTab::Skill);DrawPixels(stage,canvas);
    const auto rejectedBefore=target->GetEquippedSkillIDs();
    const int rejectedMP=target->GetStat().CurMP,rejectedGold=game.GetGold();
    const std::array<double,3> previousSlotPulse{stage.GetSkillPulseStrengthForSlot(0),stage.GetSkillPulseStrengthForSlot(1),stage.GetSkillPulseStrengthForSlot(2)};
    std::vector<std::pair<int,double>> previousLearnedPulse;
    for(int id:target->GetLearnedSkillIDs())previousLearnedPulse.push_back({id,stage.GetLearnedSkillPulseStrength(id)});
    for(const auto category:std::array<std::pair<ECharacterClass,EElement>,2>{{
        {ECharacterClass::Mage,EElement::Fire},{ECharacterClass::Warrior,EElement::Water}}}){
        const auto foreignSkills=SkillData::GetSkillIDs(category.first,category.second);
        Require(!foreignSkills.empty()&&!SkillData::CanCharacterUse(target,foreignSkills[0]),"Rejected-skill fixture unexpectedly satisfies the actor's class/element rules");
        target->AddSkillID(foreignSkills[0]);
        const auto expanded=target->GetLearnedSkillIDs();
        Require(!expanded.empty()&&expanded.back()==foreignSkills[0],"Cannot prepare an externally learned but unusable skill");
        DrawPixels(stage,canvas);Click(stage,stage.GetLearnedSkillRect(static_cast<int>(expanded.size())-1));
        Require(target->GetEquippedSkillIDs()==rejectedBefore&&stage.GetSelectedSkillSlot()==2,"Rejected skill changed the actual equipped skills or selected slot");
        Require(!stage.IsSkillFeedbackTransitioning()&&stage.GetLearnedSkillPulseStrength(foreignSkills[0])==0,
            "Rejected class/element skill started a false replacement fade or success pulse");
        for(int i=0;i<3;++i)Require(stage.GetSkillPulseStrengthForSlot(i)==previousSlotPulse[i],"Rejected skill changed an existing slot's success highlight");
        for(const auto& pulse:previousLearnedPulse)Require(stage.GetLearnedSkillPulseStrength(pulse.first)==pulse.second,"Rejected skill changed another learned card's highlight");
        Require(target->GetStat().CurMP==rejectedMP&&game.GetGold()==rejectedGold,"Rejected skill consumed MP or gold");
    }
    Geometry(stage,client);
    metadata<<"\n  ]\n}\n";Require(bool(metadata),"Cannot save action-feedback checkpoint metadata");
}
void CheckShopFeedback(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Skill);
    Canvas canvas(client.right,client.bottom);
    Click(stage,stage.GetSkillSlotRect(2));Click(stage,stage.GetPartySelectRect(2));stage.AdvanceTabPresentation(.16);
    const int selectedParty=stage.GetSelectedPartyIndex(),selectedSlot=stage.GetSelectedSkillSlot();
    auto* target=game.GetParty()[selectedParty];const auto equippedSkills=target->GetEquippedSkillIDs();
    const auto equipment=target->GetEquipment();const int mp=target->GetStat().CurMP;
    Tab(stage,EMaintenanceTab::Shop);DrawPixels(stage,canvas);
    const RECT panel=stage.GetMainPanelRect();
    const RECT tabs{panel.left,panel.top+102,panel.right,panel.top+149};
    const RECT shopBody{panel.left+16,panel.top+150,panel.right-16,panel.bottom-8};
    auto indexOf=[&](int id){const auto items=stage.GetShopItemIDs();const auto found=std::find(items.begin(),items.end(),id);
        Require(found!=items.end(),"Shop feedback fixture item is absent from the actual tier catalog");return static_cast<int>(found-items.begin());};
    auto clickItem=[&](int id){return Click(stage,stage.GetShopSlotRect(indexOf(id)));};
    auto unchangedSelection=[&](){
        Require(stage.GetSelectedPartyIndex()==selectedParty&&stage.GetSelectedSkillSlot()==selectedSlot&&target->GetEquippedSkillIDs()==equippedSkills,
            "Shop purchase feedback changed retained character or skill selection");
        const auto current=target->GetEquipment();
        Require(current.WeaponID==equipment.WeaponID&&current.ArmorID==equipment.ArmorID&&current.AccessoryID==equipment.AccessoryID&&target->GetStat().CurMP==mp,
            "Buying an item equipped it automatically or consumed character MP");
    };
    auto lineFits=[&](const std::wstring& text,int fontHeight,int width){
        const int saved=SaveDC(canvas.dc);HFONT font=CreateFontW(-fontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Malgun Gothic");
        Require(font!=nullptr,"Cannot measure native shop feedback text");SelectObject(canvas.dc,font);SIZE extent{};
        const bool measured=GetTextExtentPoint32W(canvas.dc,text.c_str(),static_cast<int>(text.size()),&extent)!=FALSE;
        RestoreDC(canvas.dc,saved);DeleteObject(font);
        Require(measured&&extent.cx<=width,"Shop text exceeds its reserved single-line width");
    };
    for(int tier=1;tier<=3;++tier){
        game.MaxUnlockedBattleStageId=tier;const auto catalog=stage.GetShopItemIDs();
        Require(catalog.size()==(tier==1?4:tier==2?8:10),"Shop feedback changed tier availability");
        const auto text=RenderText(stage,canvas);Has(text,L"Shop Tier: "+std::to_wstring(tier),"Shop feedback removed the live tier label");
        for(size_t i=0;i<catalog.size();++i){
            const int id=catalog[i];const RECT card=stage.GetShopSlotRect(static_cast<int>(i));
            Require(Inside(panel,card)&&card.bottom-card.top>=43,"Shop card cannot contain both information rows");
            for(size_t j=0;j<i;++j)Require(!Overlap(card,stage.GetShopSlotRect(static_cast<int>(j))),"Shop feedback made purchase hit rectangles overlap");
            Has(text,ItemData::GetName(id),"Shop feedback removed an item name");
            Has(text,std::to_wstring(ItemData::GetPrice(id))+L"G","Shop feedback removed an item price");
            lineFits(ItemData::GetName(id),18,card.right-card.left-130);
            const std::wstring type=ItemData::IsConsumable(id)?L"[Use]":L"[Equip]";
            Has(text,type,"Shop feedback removed the item use/equipment action type");
            lineFits(type+L"    Bag: 0",15,card.right-card.left-92);
            lineFits(std::to_wstring(ItemData::GetPrice(id))+L"G",18,96);
        }
        Has(text,L"Bag: 0","Initially empty inventory quantity is not shown in the shop");
    }
    game.MaxUnlockedBattleStageId=3;

    // No money: one attempted click must leave every quantity unchanged and
    // describe the exact shortfall, without a fictitious deduction/+1 badge.
    Require(game.GetGold()==0&&game.GetInventory().empty(),"Shop feedback initial fixture is not empty");
    clickItem(101);
    Require(stage.IsShopFeedbackActive()&&stage.GetShopFeedbackItemID()==101&&stage.GetShopShortfall()==50,
        "Zero-gold attempt did not identify the actual item and missing gold");
    Require(stage.GetShopGoldDelta()==0&&stage.GetShopQuantityDelta()==0&&game.GetGold()==0&&game.GetInventory().empty(),
        "Failed purchase produced false money/quantity changes");
    const auto failureStart=DrawPixels(stage,canvas);stage.AdvanceTabPresentation(.08);
    Require(stage.GetShopNoticeOpacity()>0&&stage.GetShopNoticeOpacity()<1,"Shop notice did not pass through an intermediate entry opacity");
    const auto failureMiddle=DrawPixels(stage,canvas);canvas.Save(output,"shop-feedback-insufficient-middle");
    const std::wstring smallPotionFailure=L"Cannot buy Small Potion. Not enough gold or invalid item.";
    auto text=RenderText(stage,canvas);Has(text,smallPotionFailure,"Insufficient-gold notice lost its existing explanatory text");
    Has(text,L"Need 50G more.","Insufficient-gold notice lost its exact shortfall");
    lineFits(smallPotionFailure,17,panel.right-panel.left-56-260);
    const double failedProgress=stage.GetShopFeedbackProgress();Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
    Require(stage.GetShopFeedbackProgress()==failedProgress,"Clicking the current Shop tab restarted its notice");
    stage.AdvanceTabPresentation(.08);const auto failureComplete=DrawPixels(stage,canvas);
    Require(stage.GetShopNoticeOpacity()>.99&&DifferentPixels(failureStart,failureMiddle,canvas.width,shopBody)>10,
        "Shop notice skipped its visible fade-in");
    Require(DifferentPixels(failureStart,failureComplete,canvas.width,tabs)==0,"Shop notice moved or blended tab controls");
    unchangedSelection();stage.AdvanceTabPresentation(2.44);
    Require(!stage.IsShopFeedbackActive()&&stage.GetShopNoticeOpacity()==0&&stage.GetShopGoldDelta()==0&&stage.GetShopQuantityDelta()==0&&stage.GetShopShortfall()==0,
        "Expired shop notice retained stale transaction values");

    // Exact price, repeated same-item clicks, then a different item all use the
    // original one-purchase-per-click accounting while presentation is active.
    game.SetGold(50);clickItem(101);
    Require(game.GetGold()==0&&game.GetItemCount(101)==1&&stage.GetShopGoldDelta()==-50&&stage.GetShopQuantityDelta()==1&&stage.GetShopShortfall()==0,
        "Exact-price purchase did not spend exactly its price and add one item");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"shop-feedback-exact-price");
    text=RenderText(stage,canvas);Has(text,L"Bag: 1","Purchased quantity is not shown immediately");Has(text,L"-50G","Actual gold deduction badge missing");
    Has(text,L"+1","Actual purchased-quantity badge missing");unchangedSelection();stage.AdvanceTabPresentation(2.44);
    game.SetGold(250);DrawPixels(stage,canvas);clickItem(101);stage.AdvanceTabPresentation(.05);DrawPixels(stage,canvas);clickItem(101);
    Require(game.GetGold()==150&&game.GetItemCount(101)==3&&stage.GetShopGoldDelta()==-50&&stage.GetShopQuantityDelta()==1,
        "Rapid repeated purchase was dropped, duplicated or showed a false transaction delta");
    stage.AdvanceTabPresentation(.05);DrawPixels(stage,canvas);clickItem(102);
    Require(game.GetGold()==80&&game.GetItemCount(101)==3&&game.GetItemCount(102)==1&&stage.GetShopFeedbackItemID()==102&&stage.GetShopGoldDelta()==-70&&stage.GetShopQuantityDelta()==1,
        "Rapid different-item purchase did not retain the latest actual transaction");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"shop-feedback-rapid-purchases");
    text=RenderText(stage,canvas);Has(text,L"Bag: 3","Repeated purchases lost the cumulative bag quantity");Has(text,L"Bag: 1","Different-item quantity disappeared");
    clickItem(201);
    Require(game.GetGold()==80&&game.GetItemCount(201)==0&&game.GetItemCount(101)==3&&game.GetItemCount(102)==1&&stage.GetShopFeedbackItemID()==201&&stage.GetShopShortfall()==120,
        "Failure after successful purchases altered inventory/gold or calculated the wrong shortfall");
    Require(stage.GetShopGoldDelta()==0&&stage.GetShopQuantityDelta()==0,"Failure reused a previous success delta");
    stage.AdvanceTabPresentation(.16);text=RenderText(stage,canvas);Has(text,L"Need 120G more.","Updated failure notice displayed the previous shortfall");
    Has(text,L"Cannot buy Iron Sword. Not enough gold or invalid item.","Failed sword purchase lost its explanatory notice");
    lineFits(L"Cannot buy Iron Sword. Not enough gold or invalid item.",17,panel.right-panel.left-56-260);unchangedSelection();
    const int unchangedGold=game.GetGold();const auto failureProgress=stage.GetShopFeedbackProgress();
    Click(stage,{panel.left+18,panel.top+190,panel.left+24,panel.top+196});
    Require(game.GetGold()==unchangedGold&&stage.GetShopFeedbackProgress()==failureProgress,"Click outside shop cards invented a transaction");

    // Tab changes clear local feedback but preserve actual bag contents and
    // original character/skill selection. The existing tab guard still blocks
    // hidden purchase cards until their incoming body becomes visible.
    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Inventory));
    Require(!stage.IsShopFeedbackActive()&&stage.GetShopFeedbackItemID()==-1&&stage.GetShopGoldDelta()==0&&stage.GetShopQuantityDelta()==0,
        "Leaving Shop leaked an outgoing transaction highlight");
    stage.AdvanceTabPresentation(.18);text=RenderText(stage,canvas);
    Has(text,L"Small Potion x3","Inventory return lost purchased potion quantity");Has(text,L"Mana Potion x1","Inventory return lost purchased mana quantity");
    Shot(stage,canvas,output,"shop-feedback-inventory-return");unchangedSelection();
    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
    Require(stage.IsTabTransitioning(),"Shop return did not retain the existing body transition");
    clickItem(101);
    Require(game.GetGold()==80&&game.GetItemCount(101)==3&&!stage.IsShopFeedbackActive(),"Incoming Shop transition accepted a hidden purchase");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);clickItem(101);
    Require(game.GetGold()==30&&game.GetItemCount(101)==4&&stage.GetShopGoldDelta()==-50&&stage.GetShopQuantityDelta()==1,
        "Settled Shop did not restore normal purchase input");
    stage.AdvanceTabPresentation(2.4);
    Require(stage.GetShopNoticeOpacity()>0&&stage.GetShopNoticeOpacity()<1,"Shop notice skipped its timed fade-out");
    stage.AdvanceTabPresentation(.2);
    Require(!stage.IsShopFeedbackActive()&&stage.GetShopFeedbackItemID()==-1&&stage.GetShopNoticeOpacity()==0,
        "Shop notice did not finish after its fade-out");
    Require(game.GetGold()==30&&game.GetItemCount(101)==4&&game.GetItemCount(102)==1,"Notice expiration changed real resources");
    unchangedSelection();Geometry(stage,client);
}
void CheckInventoryPotionFeedback(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Skill);
    Canvas canvas(client.right,client.bottom);Click(stage,stage.GetSkillSlotRect(2));
    auto* target=game.GetParty()[0];auto stats=target->GetStat();stats.CurHP=20;stats.CurMP=stats.MaxMP-5;target->SetStat(stats);
    game.SetGold(275);game.AddItem(101,5);game.AddItem(102,3);game.AddItem(103,1);
    const auto originalEquipment=target->GetEquipment();const auto originalSkills=target->GetEquippedSkillIDs();
    const int selectedSlot=stage.GetSelectedSkillSlot();
    auto inventoryFooterFits=[&](int lastIndex){
        const RECT panel=stage.GetMainPanelRect();
        const RECT guide{panel.left+28,panel.bottom-62,panel.right-28,panel.bottom-44};
        const RECT notice{panel.left+28,panel.bottom-39,panel.right-28,panel.bottom-12};
        Require(stage.GetInventorySlotRect(lastIndex).bottom<=guide.top&&guide.bottom<=notice.top&&Inside(panel,notice),
            "Inventory cards, usage guide and recovery notice overlap in the compact client");
    };
    auto inventoryTextFits=[&](const std::wstring& text,int fontHeight,const RECT& bounds){
        const int saved=SaveDC(canvas.dc);HFONT font=CreateFontW(-fontHeight,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Malgun Gothic");
        Require(font!=nullptr,"Cannot measure native inventory text");SelectObject(canvas.dc,font);SIZE extent{};
        const bool measured=GetTextExtentPoint32W(canvas.dc,text.c_str(),static_cast<int>(text.size()),&extent)!=FALSE;
        RestoreDC(canvas.dc,saved);DeleteObject(font);
        Require(measured&&extent.cx<=bounds.right-bounds.left&&extent.cy<=bounds.bottom-bounds.top,
            "Inventory title/count/type/delta text exceeds its reserved width or height");
    };
    auto inventoryCardTextFits=[&](){
        const auto& inventory=game.GetInventory();
        for(size_t i=0;i<inventory.size()&&i<20;++i){
            const auto& item=inventory[i];const RECT card=stage.GetInventorySlotRect(static_cast<int>(i));const bool compact=card.bottom-card.top<50;
            const int detailTop=compact?card.top+24:card.bottom-22;
            Require(card.top+23<=detailTop,"Inventory name and count rows overlap");
            inventoryTextFits(ItemData::GetName(item.ItemID),inventory.size()>10?15:compact?16:17,
                {card.left+12,card.top+3,card.right-12,card.top+23});
            const std::wstring details=L"x"+std::to_wstring(item.Count)+(ItemData::IsConsumable(item.ItemID)?L"  [Use]":L"  [Equip]");
            inventoryTextFits(details,compact?12:14,{card.left+12,detailTop,card.right-54,card.bottom-3});
            inventoryTextFits(L"-1",compact?12:14,{card.right-50,detailTop,card.right-12,card.bottom-3});
        }
    };
    auto clickPotion=[&](int id){return Click(stage,stage.GetInventorySlotRect(InventoryIndex(id)));};
    auto selectionPreserved=[&](){
        const auto equipment=target->GetEquipment();
        Require(game.GetGold()==275&&stage.GetSelectedSkillSlot()==selectedSlot&&target->GetEquippedSkillIDs()==originalSkills,
            "Potion feedback changed gold, skill assignment or retained skill slot");
        Require(equipment.WeaponID==originalEquipment.WeaponID&&equipment.ArmorID==originalEquipment.ArmorID&&equipment.AccessoryID==originalEquipment.AccessoryID,
            "Potion feedback changed equipped items");
        Require(!stage.IsShopFeedbackActive()&&!stage.IsEquipmentFeedbackActive(),"Potion use created unrelated shop/equipment feedback");
    };
    Tab(stage,EMaintenanceTab::Inventory);DrawPixels(stage,canvas);
    clickPotion(101);
    Require(target->GetStat().CurHP==70&&target->GetStat().CurMP==stats.CurMP&&game.GetItemCount(101)==4,
        "Small potion changed the original heal/consume semantics");
    Require(stage.IsInventoryFeedbackActive()&&stage.WasInventoryUseSuccessful()&&stage.GetInventoryFeedbackItemID()==101&&
        stage.GetInventoryHPDelta()==50&&stage.GetInventoryMPDelta()==0&&stage.GetInventoryQuantityDelta()==-1&&stage.GetInventoryRemainingCount()==4,
        "Small potion feedback does not describe its actual HP and bag deltas");
    stage.AdvanceTabPresentation(.08);
    Require(stage.GetInventoryNoticeOpacity()>0&&stage.GetInventoryNoticeOpacity()<1,"Potion notice skipped its entrance transition");
    DrawPixels(stage,canvas);
    Require(stage.GetInventoryDisplayedHPFraction()>20.0/stats.MaxHP&&stage.GetInventoryDisplayedHPFraction()<70.0/stats.MaxHP,
        "HP bar snapped to the new amount instead of passing through an intermediate fill");
    const double progress=stage.GetInventoryFeedbackProgress();Click(stage,stage.GetTabRect(EMaintenanceTab::Inventory));
    Require(stage.GetInventoryFeedbackProgress()==progress,"Selecting the current Inventory tab restarted its potion feedback");
    stage.AdvanceTabPresentation(.08);Shot(stage,canvas,output,"inventory-potion-hp-restored");
    auto text=RenderText(stage,canvas);Has(text,L"Small Potion","Potion result lost the consumed item name");
    Has(text,L"HP","Inventory feedback lost the target's HP information");Has(text,L"MP","Inventory feedback lost the target's MP information");
    selectionPreserved();

    // A rapid HP -> MP use reports the latest actual capped recovery, not the
    // item description's nominal amount or the preceding item's HP delta.
    const double hpBeforeRetarget=stage.GetInventoryDisplayedHPFraction(),mpBeforeRetarget=stage.GetInventoryDisplayedMPFraction();
    clickPotion(102);
    Require(target->GetStat().CurMP==stats.MaxMP&&game.GetItemCount(102)==2&&stage.GetInventoryMPDelta()==5&&
        stage.GetInventoryHPDelta()==0&&stage.GetInventoryQuantityDelta()==-1&&stage.GetInventoryFeedbackItemID()==102,
        "Capped mana recovery showed nominal healing or stale HP feedback");
    DrawPixels(stage,canvas);
    Require(std::abs(stage.GetInventoryDisplayedHPFraction()-hpBeforeRetarget)<1e-9&&std::abs(stage.GetInventoryDisplayedMPFraction()-mpBeforeRetarget)<1e-9,
        "Rapid potion use restarted resource bars from an old or final fill instead of the last displayed fill");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"inventory-potion-capped-mp");
    clickPotion(102);
    Require(stage.WasInventoryUseSuccessful()&&target->GetStat().CurMP==stats.MaxMP&&game.GetItemCount(102)==1&&
        stage.GetInventoryMPDelta()==0&&stage.GetInventoryHPDelta()==0&&stage.GetInventoryQuantityDelta()==-1,
        "Full-MP potion did not preserve the approved consume-one, recover-zero behavior");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"inventory-potion-full-mp-consumed");
    text=RenderText(stage,canvas);Has(text,L"MP +0 (already full)","Full-MP use was not described as zero recovery");
    Has(text,L"Bag: 1 (-1)","Full-MP notice did not explain the consumed potion");
    const int beforeLarge=target->GetStat().CurHP,largeHeal=(std::min)(ItemData::GetHealHP(103),target->GetStat().MaxHP-beforeLarge);
    clickPotion(103);
    Require(target->GetStat().CurHP==beforeLarge+largeHeal&&game.GetItemCount(103)==0&&stage.GetInventoryHPDelta()==largeHeal&&stage.GetInventoryRemainingCount()==0,
        "Large potion recovery or last-item depletion differs from the actual capped result");
    clickPotion(101);
    Require(target->GetStat().CurHP==target->GetStat().MaxHP&&game.GetItemCount(101)==3&&stage.WasInventoryUseSuccessful()&&
        stage.GetInventoryHPDelta()==0&&stage.GetInventoryMPDelta()==0&&stage.GetInventoryQuantityDelta()==-1,
        "Full-HP potion was incorrectly rejected/refunded or displayed a false heal");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"inventory-potion-full-hp-consumed");
    text=RenderText(stage,canvas);Has(text,L"HP +0 (already full)","Full-HP use was not described as zero recovery");
    Has(text,L"Bag: 3 (-1)","Full-HP notice did not explain the consumed potion");selectionPreserved();
    const RECT panel=stage.GetMainPanelRect();inventoryTextFits(L"Used Small Potion. HP +0 (already full). Bag: 3 (-1).",16,{panel.left+28,panel.bottom-39,panel.right-28,panel.bottom-12});

    // Character targeting in Inventory stays immediate and clears only the
    // outgoing actor's feedback, retaining the shared selected skill slot.
    const auto otherBefore=game.GetParty()[1]->GetStat();Click(stage,stage.GetPartySelectRect(1));
    Require(stage.GetSelectedPartyIndex()==1&&!stage.IsInventoryFeedbackActive()&&stage.GetInventoryFeedbackItemID()==-1&&stage.GetSelectedSkillSlot()==selectedSlot,
        "Inventory target switch leaked the previous actor's recovery feedback or lost slot selection");
    Require(game.GetParty()[1]->GetStat().CurHP==otherBefore.CurHP&&game.GetParty()[1]->GetStat().CurMP==otherBefore.CurMP,
        "Changing the potion target applied recovery without using a potion");
    Click(stage,stage.GetPartySelectRect(0));DrawPixels(stage,canvas);clickPotion(101);
    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Shop));
    Require(!stage.IsInventoryFeedbackActive()&&stage.GetInventoryHPDelta()==0&&stage.GetInventoryMPDelta()==0&&stage.GetInventoryQuantityDelta()==0,
        "Leaving Inventory leaked potion deltas to another tab");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Inventory));
    const int beforeGuard=game.GetItemCount(101);clickPotion(101);
    Require(game.GetItemCount(101)==beforeGuard&&!stage.IsInventoryFeedbackActive(),"Incoming Inventory tab accepted a hidden potion click");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);clickPotion(101);
    Require(game.GetItemCount(101)==beforeGuard-1&&stage.WasInventoryUseSuccessful(),"Completed Inventory transition did not restore potion input");selectionPreserved();

    // A normal depleted row must never transfer its item highlight to the
    // different item that shifts into its old slot index.
    Tab(stage,EMaintenanceTab::Shop);game.ClearInventory();game.AddItem(101,1);game.AddItem(102,1);game.AddItem(201,1);
    Tab(stage,EMaintenanceTab::Inventory);DrawPixels(stage,canvas);clickPotion(101);stage.AdvanceTabPresentation(.16);
    Require(game.GetInventory().size()==2&&game.GetInventory()[0].ItemID==102&&stage.GetInventoryFeedbackItemID()==101&&stage.GetInventoryRemainingCount()==0,
        "Depleted potion feedback was rebound to the shifted inventory row");
    const RECT shiftedCard=stage.GetInventorySlotRect(0);const auto depletedPulse=DrawPixels(stage,canvas);
    stage.AdvanceTabPresentation(2.44);const auto depletedSettled=DrawPixels(stage,canvas);
    Require(DifferentPixels(depletedPulse,depletedSettled,canvas.width,shiftedCard)==0,"Depleted potion flashed the next item's card by slot index");

    // The existing catalog has only ten IDs. This explicit expanded-layout
    // fixture adds a valid duplicate equipment row solely to exercise the
    // existing >10-row compact layout; it is not a normal AddItem/save state.
    game.ClearInventory();for(const auto& item:ItemData::GetAllItems())game.AddItem(item.ItemID,1);
    game.Inventory.push_back(InventoryItem(201,1));
    Require(game.GetInventory().size()==11,"Expanded-layout fixture no longer crosses the four-column threshold");
    const RECT compactFirst=stage.GetInventorySlotRect(0);
    std::vector<RECT> compactCards;for(int i=0;i<11;++i){const RECT card=stage.GetInventorySlotRect(i);Require(Inside(stage.GetMainPanelRect(),card),"Compact inventory card escapes the panel");
        for(const auto& previous:compactCards)Require(!Overlap(card,previous),"Compact inventory hit rectangles overlap");compactCards.push_back(card);}
    text=RenderText(stage,canvas);for(const auto& item:ItemData::GetAllItems())Has(text,ItemData::GetName(item.ItemID),"Compact inventory lost an existing item name");
    const auto elevenRows=game.Inventory;
    while(game.Inventory.size()<20)game.Inventory.push_back(InventoryItem(201,1));
    inventoryFooterFits(19);inventoryCardTextFits();game.Inventory=elevenRows;
    DrawPixels(stage,canvas);clickPotion(101);stage.AdvanceTabPresentation(.16);
    const RECT wideFirst=stage.GetInventorySlotRect(0);
    Require(game.GetInventory().size()==10&&wideFirst.right-wideFirst.left>compactFirst.right-compactFirst.left&&game.GetInventory()[0].ItemID==102,
        "Depletion did not retain the original four-to-two-column inventory reflow");
    inventoryFooterFits(9);inventoryCardTextFits();
    const auto reflowPulse=DrawPixels(stage,canvas);stage.AdvanceTabPresentation(2.44);const auto reflowSettled=DrawPixels(stage,canvas);
    Require(DifferentPixels(reflowPulse,reflowSettled,canvas.width,wideFirst)==0,"Inventory reflow applied the depleted item's pulse to another item");

    // Last potion: the empty-state label and completed usage result coexist.
    game.ClearInventory();game.AddItem(101,1);DrawPixels(stage,canvas);clickPotion(101);stage.AdvanceTabPresentation(.16);
    Require(game.GetInventory().empty()&&stage.IsInventoryFeedbackActive()&&stage.WasInventoryUseSuccessful()&&stage.GetInventoryQuantityDelta()==-1&&stage.GetInventoryRemainingCount()==0,
        "Last potion erased its result or failed to remove the consumed inventory row");
    text=RenderText(stage,canvas);Has(text,L"No Items Found.","Empty-inventory state disappeared after the last potion");Has(text,L"Small Potion","Empty-state early return hid the last potion's result");
    Shot(stage,canvas,output,"inventory-potion-last-item-empty");
    const auto emptyStats=target->GetStat();const double emptyProgress=stage.GetInventoryFeedbackProgress();Click(stage,stage.GetInventorySlotRect(0));
    Require(game.GetInventory().empty()&&stage.GetInventoryFeedbackProgress()==emptyProgress&&target->GetStat().CurHP==emptyStats.CurHP&&target->GetStat().CurMP==emptyStats.CurMP,
        "Clicking an empty inventory location invented another potion use");
    stage.AdvanceTabPresentation(2.24);Require(stage.GetInventoryNoticeOpacity()>0&&stage.GetInventoryNoticeOpacity()<1,"Potion result skipped its fade-out");
    stage.AdvanceTabPresentation(.20);Require(!stage.IsInventoryFeedbackActive()&&stage.GetInventoryNoticeOpacity()==0,"Potion result did not expire independently of actual stats");

    // These malformed inventory/party fixtures are explicit defensive cases,
    // not normal AddItem states. Exercise their real public Input routing
    // instead of linking test calls to a private implementation helper.
    game.AddItem(201,1);game.AddItem(101,1);const int defenseGold=game.GetGold();
    for(int invalid:{102,99999}){
        game.Inventory.push_back(InventoryItem(invalid,invalid==102?0:1));
        const auto before=target->GetStat();const int beforeHP=before.CurHP,beforeMP=before.CurMP;
        Click(stage,stage.GetInventorySlotRect(static_cast<int>(game.GetInventory().size())-1));
        Require(!stage.WasInventoryUseSuccessful()&&stage.GetInventoryHPDelta()==0&&stage.GetInventoryMPDelta()==0&&stage.GetInventoryQuantityDelta()==0,
            "Defensive malformed-row click produced false recovery or consumption feedback");
        Require(target->GetStat().CurHP==beforeHP&&target->GetStat().CurMP==beforeMP&&game.GetGold()==defenseGold&&game.GetItemCount(201)==1&&game.GetItemCount(101)==1,
            "Defensive malformed-row click changed resources");
        game.Inventory.pop_back();
    }
    const auto activeParty=game.GetParty();game.GetParty().clear();clickPotion(101);
    Require(!stage.WasInventoryUseSuccessful()&&stage.GetInventoryHPDelta()==0&&stage.GetInventoryMPDelta()==0&&stage.GetInventoryQuantityDelta()==0&&game.GetItemCount(101)==1,
        "No-party potion click consumed inventory or reported healing");
    game.GetParty()=activeParty;
    // A real equipment card must still follow Equip, never a failed-potion path.
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(201)));
    Require(stage.GetActiveTab()==EMaintenanceTab::Equipment&&target->GetEquipment().WeaponID==201&&game.GetItemCount(201)==0&&!stage.IsInventoryFeedbackActive(),
        "Non-consumable equipment click stopped using its original equip route");
    stage.AdvanceTabPresentation(.18);Click(stage,stage.GetEquipmentSlotRect(0));
    Require(target->GetEquipment().WeaponID==-1&&game.GetItemCount(201)==1,"Equipment restoration after defensive potion checks failed");
    Tab(stage,EMaintenanceTab::Inventory);
    selectionPreserved();Geometry(stage,client);
}
void CheckPartyFeedback(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();
    // First draw and enhanced-metafile capture cannot supply a readable source
    // bitmap. The logical toggle and timed message still have to work.
    game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    {
        OffscreenMaintenanceStage fresh;PrepareMaintenance(fresh,client);Tab(fresh,EMaintenanceTab::Party);
        Canvas firstCanvas(client.right,client.bottom);RenderText(fresh,firstCanvas);
        Click(fresh,fresh.GetRosterSlotRect(1));
        Require(game.GetParty().size()==4&&fresh.IsPartyFeedbackActive()&&fresh.WasPartyChangeSuccessful()&&fresh.GetPartyCountDelta()==-1,
            "First-draw party toggle lost its actual action or message");
        Require(!fresh.IsPartyPresentationTransitioning(),"First-draw party transition used a nonexistent source bitmap");
        fresh.AdvanceTabPresentation(.08);const auto text=RenderText(fresh,firstCanvas);
        Has(text,L"Removed Water Warrior from the active party.","First-draw party result is missing from actual GDI text");
        Click(fresh,fresh.GetRosterSlotRect(1));
        Require(game.GetParty().size()==5&&!fresh.IsPartyPresentationTransitioning(),"Metafile result capture incorrectly became a readable party transition source");
        fresh.AdvanceTabPresentation(2.6);Require(!fresh.IsPartyFeedbackActive(),"First-draw party result did not expire");
    }
    game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();game.SetGold(275);game.AddItem(101,2);
    game.AddItem(201,1);Require(game.EquipItem(game.GetParty()[0],201),"Cannot prepare actual equipment for party-preservation checks");
    for(auto* actor:game.GetRoster()){auto stat=actor->GetStat();stat.CurHP=(std::max)(1,stat.MaxHP-5);stat.CurMP=(std::max)(0,stat.MaxMP-3);actor->SetStat(stat);}
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Skill);Canvas canvas(client.right,client.bottom);
    Click(stage,stage.GetSkillSlotRect(2));Click(stage,stage.GetPartySelectRect(2));stage.AdvanceTabPresentation(.16);
    Click(stage,stage.GetLearnedSkillRect(0));
    const std::wstring originalSkillMessage=L"Equipped "+SkillData::GetName(game.GetParty()[2]->GetLearnedSkillIDs()[0])+L" to slot 3.";
    const int retainedSlot=stage.GetSelectedSkillSlot();Has(RenderText(stage,canvas),originalSkillMessage,"Skill assignment did not prepare its visible feedback");
    Require(!originalSkillMessage.empty()&&stage.GetSelectedPartyIndex()==2,"Cannot prepare shared selected character/skill message through input");
    std::vector<CharacterStat> baselineStats;std::vector<CharacterEquipment> baselineEquipment;std::vector<std::vector<int>> baselineSkills;
    for(auto* actor:game.GetRoster()){baselineStats.push_back(actor->GetStat());baselineEquipment.push_back(actor->GetEquipment());baselineSkills.push_back(actor->GetEquippedSkillIDs());}
    auto preserveData=[&](){
        Require(game.GetGold()==275&&game.GetItemCount(101)==2&&game.GetItemCount(201)==0&&stage.GetSelectedSkillSlot()==retainedSlot,
            "Party editing changed gold, inventory, or the selected skill slot");
        const auto& roster=game.GetRoster();Require(roster.size()==baselineStats.size(),"Party editing changed the owned roster");
        for(size_t i=0;i<roster.size();++i){const auto stat=roster[i]->GetStat();const auto& old=baselineStats[i];const auto equip=roster[i]->GetEquipment();
            Require(stat.CurHP==old.CurHP&&stat.MaxHP==old.MaxHP&&stat.CurMP==old.CurMP&&stat.MaxMP==old.MaxMP&&stat.STR==old.STR&&stat.INT==old.INT&&stat.DEF==old.DEF&&stat.SPD==old.SPD,
                "Party editing changed a character's combat stats or resources");
            Require(equip.WeaponID==baselineEquipment[i].WeaponID&&equip.ArmorID==baselineEquipment[i].ArmorID&&equip.AccessoryID==baselineEquipment[i].AccessoryID&&roster[i]->GetEquippedSkillIDs()==baselineSkills[i],
                "Party editing changed equipment or equipped skills");}
    };
    auto clickRoster=[&](int id){const auto& roster=game.GetRoster();
        const auto found=std::find_if(roster.begin(),roster.end(),[&](Character* actor){return actor->GetCharacterID()==id;});
        Require(found!=roster.end(),"Party feedback fixture actor is absent from the actual roster");
        return Click(stage,stage.GetRosterSlotRect(static_cast<int>(found-roster.begin())));};
    Tab(stage,EMaintenanceTab::Party);const RECT panel=stage.GetMainPanelRect(),first=stage.GetRosterSlotRect(0);
    const int rowHeight=first.bottom-first.top;
    const RECT list{panel.left+28,first.top,panel.left+304,first.top+5*(rowHeight+8)-8};
    const RECT tabs{panel.left,panel.top+102,panel.right,panel.top+149};
    auto text=RenderText(stage,canvas);Has(text,L"5 / 5","Initial active-party count disappeared");
    Require(text.find(originalSkillMessage)==std::wstring::npos,"Party tab displays stale skill-assignment feedback");
    DrawPixels(stage,canvas);clickRoster(15);
    Require(game.GetParty().size()==5&&stage.GetSelectedPartyIndex()==2&&stage.IsPartyFeedbackActive()&&!stage.WasPartyChangeSuccessful()&&stage.GetPartyCountDelta()==0&&stage.GetPartyFeedbackCharacterID()==15,
        "Full-party rejection changed membership/selection or invented a count delta");
    Has(RenderText(stage,canvas),L"Active party is already full.","Full-party rejection changed the original user explanation");
    stage.AdvanceTabPresentation(.08);Require(stage.GetPartyNoticeOpacity()>0&&stage.GetPartyNoticeOpacity()<1,"Party notice did not pass through a fade-in midpoint");
    Shot(stage,canvas,output,"party-feedback-full-rejected");preserveData();
    stage.AdvanceTabPresentation(.08);const double sameTabProgress=stage.GetPartyFeedbackProgress();Click(stage,stage.GetTabRect(EMaintenanceTab::Party));
    Require(stage.GetPartyFeedbackProgress()==sameTabProgress,"Selecting the current Party tab restarted its action feedback");
    stage.AdvanceTabPresentation(2.44);Require(!stage.IsPartyFeedbackActive()&&stage.GetPartyCountDelta()==0,"Party failure feedback did not expire");

    const auto beforeRemove=DrawPixels(stage,canvas);clickRoster(2);
    Require(game.GetParty().size()==4&&PartyIndex(2)<0&&stage.GetSelectedPartyIndex()==2&&game.GetParty()[2]->GetCharacterID()==11,
        "Remove changed the original numeric selection retention behavior");
    Require(stage.WasPartyChangeSuccessful()&&stage.GetPartyCountDelta()==-1&&stage.GetPartyFeedbackCharacterID()==2&&stage.IsPartyPresentationTransitioning(),
        "Successful removal did not begin its matching party presentation");
    const auto removeStart=DrawPixels(stage,canvas);Require(DifferentPixels(beforeRemove,removeStart,canvas.width,list)==0,"Party removal snapped its active list before the fade progressed");
    text=RenderText(stage,canvas);Has(text,L"4 / 5","Logical party count was delayed by the list animation");
    stage.AdvanceTabPresentation(.08);const auto removeMiddle=DrawPixels(stage,canvas);canvas.Save(output,"party-feedback-remove-middle");
    Require(stage.GetPartyPresentationProgress()>0&&stage.GetPartyPresentationProgress()<1&&DifferentPixels(removeStart,removeMiddle,canvas.width,list)>30,
        "Party removal did not produce an intermediate visible list");
    Require(DifferentPixels(removeStart,removeMiddle,canvas.width,tabs)==0,"Party action blended or moved tab controls");
    clickRoster(2);
    Require(game.GetParty().size()==5&&game.GetParty().back()->GetCharacterID()==2&&stage.GetSelectedPartyIndex()==4&&stage.GetPartyCountDelta()==1,
        "Rapid re-add failed to select the newly appended member");
    const auto rapidAdd=DrawPixels(stage,canvas);Require(DifferentPixels(removeMiddle,rapidAdd,canvas.width,list)==0,"Rapid party re-add did not continue from the last visible mixture");
    stage.AdvanceTabPresentation(.05);const auto addMiddle=DrawPixels(stage,canvas);clickRoster(2);
    Require(game.GetParty().size()==4&&stage.GetSelectedPartyIndex()==3&&stage.GetPartyCountDelta()==-1,"Rapid removal failed to clamp the selected index to the new last member");
    const auto rapidRemove=DrawPixels(stage,canvas);Require(DifferentPixels(addMiddle,rapidRemove,canvas.width,list)==0,"Second rapid party toggle jumped to an older list image");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"party-feedback-rapid-toggle-complete");
    Require(!stage.IsPartyPresentationTransitioning()&&PartyIndex(2)<0,"Rapid party sequence did not finish on its final membership");
    clickRoster(11);clickRoster(3);clickRoster(2);
    Require(game.GetParty().size()==5&&PartyIndex(11)<0&&PartyIndex(3)>=0&&PartyIndex(2)==4&&stage.GetSelectedPartyIndex()==4&&stage.GetPartyCountDelta()==1,
        "Different-member rapid edits dropped an action, duplicated membership, or lost the latest delta");
    std::set<int> unique;for(auto* actor:game.GetParty())unique.insert(actor->GetCharacterID());Require(unique.size()==game.GetParty().size(),"Rapid party edits created duplicate members");
    preserveData();stage.AdvanceTabPresentation(.16);DrawPixels(stage,canvas);

    while(game.GetParty().size()>1)clickRoster(game.GetParty().back()->GetCharacterID());
    Require(game.GetParty().size()==1&&game.GetParty()[0]->GetCharacterID()==1&&stage.GetSelectedPartyIndex()==0,"Repeated removal did not preserve the last member and valid selection");
    clickRoster(1);Require(game.GetParty().size()==1&&stage.GetSelectedPartyIndex()==0&&!stage.WasPartyChangeSuccessful()&&stage.GetPartyCountDelta()==0,
        "Last-member rejection changed membership or selection");
    Has(RenderText(stage,canvas),L"At least one active character is required.","Last-member rejection changed the original explanation");
    stage.AdvanceTabPresentation(.16);Shot(stage,canvas,output,"party-feedback-last-member-rejected");preserveData();
    const auto lastProgress=stage.GetPartyFeedbackProgress();Click(stage,{panel.left+18,panel.top+185,panel.left+24,panel.top+189});
    Require(game.GetParty().size()==1&&stage.GetPartyFeedbackProgress()==lastProgress,"Click outside roster cards generated a party action");

    DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Skill));
    Require(!stage.IsPartyFeedbackActive()&&!stage.IsPartyPresentationTransitioning()&&stage.GetPartyCountDelta()==0&&stage.GetPartyFeedbackCharacterID()==-1,
        "Leaving Party leaked its local result or list animation");
    stage.AdvanceTabPresentation(.18);text=RenderText(stage,canvas);
    Require(text.find(L"At least one active character is required.")==std::wstring::npos&&text.find(L"Active party is already full.")==std::wstring::npos,
        "Party membership errors leaked into the Skill tab");
    Has(text,originalSkillMessage,"Party editing overwrote the previous skill-assignment feedback");
    preserveData();DrawPixels(stage,canvas);Click(stage,stage.GetTabRect(EMaintenanceTab::Party));
    clickRoster(2);Require(game.GetParty().size()==1&&!stage.IsPartyFeedbackActive(),"Incoming Party tab accepted an obscured roster click");
    stage.AdvanceTabPresentation(.18);DrawPixels(stage,canvas);clickRoster(2);
    Require(game.GetParty().size()==2&&stage.GetSelectedPartyIndex()==1&&stage.GetPartyCountDelta()==1,"Settled Party tab did not restore ordinary roster input");
    stage.AdvanceTabPresentation(2.4);Require(stage.GetPartyNoticeOpacity()>0&&stage.GetPartyNoticeOpacity()<1,"Party result skipped its exit fade");
    stage.AdvanceTabPresentation(.2);Require(!stage.IsPartyFeedbackActive()&&stage.GetPartyNoticeOpacity()==0,"Party result did not finish at its timed expiration");
    preserveData();Geometry(stage,client);

    const RECT last=stage.GetRosterSlotRect(14),guide{panel.left+28,panel.bottom-57,panel.right-28,panel.bottom-39},notice{panel.left+28,panel.bottom-34,panel.right-28,panel.bottom-10};
    Require(last.bottom<=guide.top&&guide.bottom<=notice.top&&Inside(panel,notice),"Party roster, instructions and result notice overlap in a compact client");
    auto partyTextFits=[&](const std::wstring& value,int fontSize,const RECT& bounds){
        const int saved=SaveDC(canvas.dc);HFONT font=CreateFontW(-fontSize,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
            DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Malgun Gothic");
        Require(font!=nullptr,"Cannot measure native Party text");SelectObject(canvas.dc,font);SIZE extent{};
        const bool measured=GetTextExtentPoint32W(canvas.dc,value.c_str(),static_cast<int>(value.size()),&extent)!=FALSE;
        RestoreDC(canvas.dc,saved);DeleteObject(font);
        Require(measured&&extent.cx<=bounds.right-bounds.left&&extent.cy<=bounds.bottom-bounds.top,"Party name/status/notice does not fit its visible text area");
    };
    for(size_t i=0;i<game.GetRoster().size();++i){const RECT card=stage.GetRosterSlotRect(static_cast<int>(i));
        partyTextFits(game.GetRoster()[i]->GetName(),16,{card.left+52,card.top+7,card.right-8,card.top+32});
        partyTextFits(L"ACTIVE",13,{card.left+52,card.top+31,card.right-8,card.top+51});}
    partyTextFits(L"At least one active character is required.",17,notice);
    partyTextFits(L"Active party is already full.",17,notice);
    partyTextFits(L"Removed Water Warrior from the active party.",17,notice);
}
void CheckContinuousMaintenanceFlow(RECT client,const std::filesystem::path& output)
{
    enum class Action { Roster, Tab, Member, InventoryItem, Unequip, SkillSlot, LearnedSkill, ShopItem };
    struct Event { double Time;Action Kind;int Argument;const char* Name; };
    double duration=0;int previewFPS=0,legacyUpdateHz=0,initialGold=0;
    std::vector<std::pair<int,int>> initialItems;std::vector<std::array<int,3>> resources;
    std::vector<Event> events;std::vector<int> finalParty;std::array<int,7> final{};
#define FLOW_CONFIG(seconds,fps,hz) duration=seconds;previewFPS=fps;legacyUpdateHz=hz;
#define FLOW_GOLD(value) initialGold=value;
#define FLOW_ITEM(id,count) initialItems.push_back({id,count});
#define FLOW_RESOURCES(id,hp,mp) resources.push_back({id,hp,mp});
#define FLOW_EVENT(seconds,kind,argument) events.push_back({seconds,Action::kind,argument,#kind});
#define FLOW_FINAL_PARTY(...) finalParty={__VA_ARGS__};
#define FLOW_FINAL(...) final={__VA_ARGS__};
#include "maintenance_full_flow_scenario.inc"
#undef FLOW_CONFIG
#undef FLOW_GOLD
#undef FLOW_ITEM
#undef FLOW_RESOURCES
#undef FLOW_EVENT
#undef FLOW_FINAL_PARTY
#undef FLOW_FINAL
    Require(duration>0&&previewFPS>0&&legacyUpdateHz>0&&!events.empty(),"Continuous scenario is missing its shared configuration");

    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    game.SetGold(initialGold);for(const auto& item:initialItems)game.AddItem(item.first,item.second);
    auto actorByID=[&](int id){const auto& roster=game.GetRoster();const auto found=std::find_if(roster.begin(),roster.end(),
        [&](Character* actor){return actor->GetCharacterID()==id;});Require(found!=roster.end(),"Continuous scenario references an unknown character");return *found;};
    for(const auto& setting:resources){auto* actor=actorByID(setting[0]);auto stat=actor->GetStat();stat.CurHP=setting[1];stat.CurMP=setting[2];actor->SetStat(stat);}
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Canvas canvas(client.right,client.bottom);
    Require(Click(stage,stage.GetTabRect(EMaintenanceTab::Party))==-2&&stage.GetActiveTab()==EMaintenanceTab::Party,
        "Continuous session could not open its initial Party tab through public input");
    DrawPixels(stage,canvas);

    // One session owns one stage and one GameManager fixture from here onward.
    // Every action uses public Input. Advance drives presentation time only;
    // the separate legacy-expiry check covers 180 public Update calls.
    enum ActorField { MaxHP,CurHP,MaxMP,CurMP,STR,INT,DEF,SPD,Weapon,Armor,Accessory,Skill0,Skill1,Skill2 };
    struct State {
        int Gold=0,Tab=0,SelectedParty=0,SelectedSlot=0;
        std::vector<int> Party,InventoryOrder;std::map<int,int> Bag;std::map<int,std::array<int,14>> Actors;
        bool operator==(const State&) const = default;
    };
    auto state=[&](){
        State value;value.Gold=game.GetGold();value.Tab=static_cast<int>(stage.GetActiveTab());value.SelectedParty=stage.GetSelectedPartyIndex();value.SelectedSlot=stage.GetSelectedSkillSlot();
        for(auto* actor:game.GetParty())value.Party.push_back(actor->GetCharacterID());
        for(const auto& item:game.GetInventory()){value.Bag[item.ItemID]=item.Count;value.InventoryOrder.push_back(item.ItemID);}
        for(auto* actor:game.GetRoster()){const auto stat=actor->GetStat();const auto gear=actor->GetEquipment();
            value.Actors[actor->GetCharacterID()]={stat.MaxHP,stat.CurHP,stat.MaxMP,stat.CurMP,stat.STR,stat.INT,stat.DEF,stat.SPD,
                gear.WeaponID,gear.ArmorID,gear.AccessoryID,actor->GetEquippedSkillID(0),actor->GetEquippedSkillID(1),actor->GetEquippedSkillID(2)};}
        return value;
    };
    auto writeState=[](std::ostream& stream,const State& value){
        stream<<"{\"gold\":"<<value.Gold<<",\"tab\":"<<value.Tab<<",\"selectedPartyIndex\":"<<value.SelectedParty<<",\"selectedSkillSlot\":"<<value.SelectedSlot<<",\"party\":[";
        for(size_t i=0;i<value.Party.size();++i){if(i)stream<<',';stream<<value.Party[i];}stream<<"],\"bag\":[";
        bool first=true;for(const auto& item:value.Bag){if(!first)stream<<',';first=false;stream<<'['<<item.first<<','<<item.second<<']';}stream<<"],\"inventoryOrder\":[";
        for(size_t i=0;i<value.InventoryOrder.size();++i){if(i)stream<<',';stream<<value.InventoryOrder[i];}stream<<"],\"actors\":[";
        first=true;for(const auto& actor:value.Actors){if(!first)stream<<',';first=false;stream<<"{\"id\":"<<actor.first<<",\"stats\":[";
            for(int i=0;i<8;++i){if(i)stream<<',';stream<<actor.second[i];}stream<<"],\"equipment\":["<<actor.second[Weapon]<<','<<actor.second[Armor]<<','<<actor.second[Accessory]<<"],\"skills\":["<<actor.second[Skill0]<<','<<actor.second[Skill1]<<','<<actor.second[Skill2]<<"]}";}
        stream<<"]}";
    };
    const State initial=state();std::vector<State> checkpoints;std::vector<bool> successful;checkpoints.reserve(events.size());successful.reserve(events.size());
    auto changeBag=[](State& value,int id,int delta){const int count=value.Bag.contains(id)?value.Bag.at(id):0;const int next=count+delta;
        Require(next>=0,"Continuous scenario expected a negative inventory count");
        if(next){value.Bag[id]=next;if(!count)value.InventoryOrder.push_back(id);}
        else {value.Bag.erase(id);value.InventoryOrder.erase(std::remove(value.InventoryOrder.begin(),value.InventoryOrder.end(),id),value.InventoryOrder.end());}};
    auto itemBonus=[](std::array<int,14>& actor,int id,int sign){
        const auto bonus=ItemData::GetBonus(id);
        actor[MaxHP]=(std::max)(1,actor[MaxHP]+bonus.MaxHP*sign);actor[MaxMP]=(std::max)(0,actor[MaxMP]+bonus.MaxMP*sign);
        actor[CurHP]=(std::max)(1,(std::min)(actor[MaxHP],actor[CurHP]+bonus.MaxHP*sign));
        actor[CurMP]=(std::max)(0,(std::min)(actor[MaxMP],actor[CurMP]+bonus.MaxMP*sign));
        actor[STR]+=bonus.STR*sign;actor[INT]+=bonus.INT*sign;actor[DEF]+=bonus.DEF*sign;actor[SPD]+=bonus.SPD*sign;
    };
    double now=0;
    auto advanceTo=[&](double time){
        Require(time>=now,"Shared continuous events are not in chronological order");const State before=state();const double gap=time-now;
        // Two checkpoints exercise the first visible transition step and the
        // settled next-event frame, without creating a video/frame renderer.
        const double first=(std::min)(.08,gap);if(first>0){stage.AdvanceTabPresentation(first);DrawPixels(stage,canvas);}
        if(gap>first){stage.AdvanceTabPresentation(gap-first);DrawPixels(stage,canvas);}
        Require(state()==before,"Presentation time alone mutated continuous-session game or selection state");now=time;
    };
    for(const auto& event:events){
        advanceTo(event.Time);Require(!stage.IsTabTransitioning()&&!stage.IsCharacterTransitioning(),"Shared scenario attempted an action before the previous view settled");
        const State before=state();State expected=before;const int selectedID=before.Party.at(before.SelectedParty);bool success=true;RECT hit{};
        switch(event.Kind){
        case Action::Roster: {
            Require(stage.GetActiveTab()==EMaintenanceTab::Party,"Roster event is not on the Party tab");
            const auto& roster=game.GetRoster();auto found=std::find_if(roster.begin(),roster.end(),[&](Character* actor){return actor->GetCharacterID()==event.Argument;});
            Require(found!=roster.end(),"Continuous roster target disappeared");hit=stage.GetRosterSlotRect(static_cast<int>(found-roster.begin()));
            auto active=std::find(expected.Party.begin(),expected.Party.end(),event.Argument);
            if(active!=expected.Party.end()){success=expected.Party.size()>1;if(success){expected.Party.erase(active);expected.SelectedParty=(std::min)(expected.SelectedParty,static_cast<int>(expected.Party.size())-1);}}
            else {success=expected.Party.size()<static_cast<size_t>(game.GetMaxActivePartySize());if(success){expected.Party.push_back(event.Argument);expected.SelectedParty=static_cast<int>(expected.Party.size())-1;}}
            break;}
        case Action::Tab:
            Require(event.Argument>=0&&event.Argument<6,"Continuous scenario tab is outside the existing enum");expected.Tab=event.Argument;hit=stage.GetTabRect(static_cast<EMaintenanceTab>(event.Argument));break;
        case Action::Member: {
            Require(stage.GetActiveTab()==EMaintenanceTab::Equipment||stage.GetActiveTab()==EMaintenanceTab::Skill||stage.GetActiveTab()==EMaintenanceTab::Inventory,"Character event has no active selector");
            const auto found=std::find(before.Party.begin(),before.Party.end(),event.Argument);Require(found!=before.Party.end(),"Continuous character target is not active");
            expected.SelectedParty=static_cast<int>(found-before.Party.begin());hit=stage.GetPartySelectRect(expected.SelectedParty);break;}
        case Action::InventoryItem: {
            Require(stage.GetActiveTab()==EMaintenanceTab::Inventory&&before.Bag.contains(event.Argument)&&before.Bag.at(event.Argument)>0,"Continuous inventory action has no visible owned item");
            hit=stage.GetInventorySlotRect(InventoryIndex(event.Argument));auto& actor=expected.Actors.at(selectedID);changeBag(expected,event.Argument,-1);
            if(ItemData::IsEquipment(event.Argument)){const int part=Weapon+static_cast<int>(ItemData::GetEquipPart(event.Argument));const int old=actor[part];
                if(old>=0){itemBonus(actor,old,-1);changeBag(expected,old,1);}actor[part]=event.Argument;itemBonus(actor,event.Argument,1);expected.Tab=static_cast<int>(EMaintenanceTab::Equipment);}
            else {Require(ItemData::IsConsumable(event.Argument),"Continuous inventory scenario contains a non-usable item");
                actor[CurHP]=(std::min)(actor[MaxHP],actor[CurHP]+ItemData::GetHealHP(event.Argument));actor[CurMP]=(std::min)(actor[MaxMP],actor[CurMP]+ItemData::GetHealMP(event.Argument));}
            break;}
        case Action::Unequip: {
            Require(stage.GetActiveTab()==EMaintenanceTab::Equipment&&event.Argument>=0&&event.Argument<3,"Continuous unequip event is not a valid equipment slot");
            hit=stage.GetEquipmentSlotRect(event.Argument);auto& actor=expected.Actors.at(selectedID);const int id=actor[Weapon+event.Argument];
            Require(id>=0,"Continuous unequip expected an occupied slot");itemBonus(actor,id,-1);actor[Weapon+event.Argument]=-1;changeBag(expected,id,1);break;}
        case Action::SkillSlot:
            Require(stage.GetActiveTab()==EMaintenanceTab::Skill&&event.Argument>=0&&event.Argument<3,"Continuous skill selection is outside the three slots");
            expected.SelectedSlot=event.Argument;hit=stage.GetSkillSlotRect(event.Argument);break;
        case Action::LearnedSkill: {
            Require(stage.GetActiveTab()==EMaintenanceTab::Skill,"Continuous skill assignment is not on the Skill tab");const auto learned=actorByID(selectedID)->GetLearnedSkillIDs();
            const auto found=std::find(learned.begin(),learned.end(),event.Argument);Require(found!=learned.end()&&SkillData::CanCharacterUse(actorByID(selectedID),event.Argument),"Continuous skill is not usable by the selected character");
            expected.Actors.at(selectedID)[Skill0+before.SelectedSlot]=event.Argument;hit=stage.GetLearnedSkillRect(static_cast<int>(found-learned.begin()));break;}
        case Action::ShopItem: {
            Require(stage.GetActiveTab()==EMaintenanceTab::Shop,"Continuous purchase is not on the Shop tab");const auto stock=stage.GetShopItemIDs();const auto found=std::find(stock.begin(),stock.end(),event.Argument);
            Require(found!=stock.end(),"Continuous purchase item is absent from the unlocked shop");hit=stage.GetShopSlotRect(static_cast<int>(found-stock.begin()));
            const int price=ItemData::GetPrice(event.Argument);success=before.Gold>=price;if(success){expected.Gold-=price;changeBag(expected,event.Argument,1);}break;}
        }
        Require(Click(stage,hit)==-2,"Continuous maintenance action requested an unintended stage change");const State after=state();
        if(!(after==expected)){std::cerr<<"Continuous mismatch at "<<event.Time<<"s "<<event.Name<<' '<<event.Argument<<'\n';Require(false,"Continuous action changed data outside its intended transaction");}
        Require(after.Actors.size()==initial.Actors.size(),"Continuous actions changed the owned roster");
        if(event.Kind==Action::Roster){Require(stage.WasPartyChangeSuccessful()==success&&stage.GetPartyCountDelta()==static_cast<int>(after.Party.size())-static_cast<int>(before.Party.size()),"Continuous party feedback does not match its actual count change");}
        if(event.Kind==Action::ShopItem){Require(stage.GetShopGoldDelta()==after.Gold-before.Gold&&stage.GetShopQuantityDelta()==(success?1:0)&&
            stage.GetShopShortfall()==(success?0:ItemData::GetPrice(event.Argument)-before.Gold),"Continuous shop feedback does not match the committed purchase or shortfall");}
        if(event.Kind==Action::InventoryItem&&ItemData::IsConsumable(event.Argument)){Require(stage.WasInventoryUseSuccessful()&&stage.GetInventoryHPDelta()==after.Actors.at(selectedID)[CurHP]-before.Actors.at(selectedID)[CurHP]&&
            stage.GetInventoryMPDelta()==after.Actors.at(selectedID)[CurMP]-before.Actors.at(selectedID)[CurMP]&&stage.GetInventoryQuantityDelta()==-1,
            "Continuous potion feedback differs from actual capped recovery/consumption");}
        if(event.Kind==Action::Tab&&before.Tab!=after.Tab)Require(!stage.IsPartyFeedbackActive()&&!stage.IsInventoryFeedbackActive()&&!stage.IsShopFeedbackActive()&&!stage.IsEquipmentFeedbackActive()&&!stage.IsSkillFeedbackTransitioning(),"Tab navigation leaked local feedback from an earlier continuous action");
        DrawPixels(stage,canvas);checkpoints.push_back(after);successful.push_back(success);
    }
    advanceTo(duration);const State result=state();
    Require(result.Party==finalParty&&result.Gold==final[0]&&result.Bag.empty()&&result.InventoryOrder.empty(),"Continuous session ended with the wrong party, gold or inventory");
    const auto& finalActor=result.Actors.at(final[1]);
    Require(finalActor[CurHP]==final[2]&&finalActor[CurMP]==final[3]&&finalActor[Armor]==final[4]&&result.SelectedSlot==final[5]&&finalActor[Skill0+final[5]]==final[6],
        "Continuous session lost equipment, recovery, selected slot or assigned skill across tabs");
    Require(result.Party.at(result.SelectedParty)==final[1]&&stage.GetActiveTab()==EMaintenanceTab::Party,"Continuous session lost its final selected character or return tab");
    Geometry(stage,client);
    // This file is emitted only when the native regression actually runs.
    // BuildOnly does not create or refresh runtime trace evidence.
    std::ofstream trace(output/("maintenance-full-flow-trace-"+std::to_string(client.right)+"x"+std::to_string(client.bottom)+".json"));
    trace<<"{\n\"scope\":\"One live in-memory fixture and stage across the shared scenario; actual public Input/Draw/Advance checkpoints. Legacy frame-count skill-message expiry is covered separately by public Update checks. No game window, native video renderer or saved-game access.\",\n"
        <<"\"durationSeconds\":"<<duration<<",\"configuredPreviewFPS\":"<<previewFPS<<",\"configuredLegacyUpdateHz\":"<<legacyUpdateHz<<",\"legacyUpdatesAppliedHere\":false,\n\"initial\":";
    writeState(trace,initial);trace<<",\n\"events\":[\n";
    for(size_t i=0;i<events.size();++i){if(i)trace<<",\n";trace<<"{\"time\":"<<events[i].Time<<",\"action\":\""<<events[i].Name<<"\",\"argument\":"<<events[i].Argument<<",\"succeeded\":"<<(successful[i]?"true":"false")<<",\"state\":";writeState(trace,checkpoints[i]);trace<<'}';}
    trace<<"\n],\n\"final\":";writeState(trace,result);trace<<"\n}\n";Require(bool(trace),"Cannot save the actually executed continuous-session trace");
}
void CheckMaintenance(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();auto& reward=BattleRewardManager::Get();
    game.Clear();game.InitDefaultParty();reward.Clear();
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);Tab(stage,EMaintenanceTab::Party);
    Canvas canvas(client.right,client.bottom);Geometry(stage,client);
    Require(game.GetRoster().size()==15&&game.GetParty().size()==5&&game.GetGold()==0&&game.GetInventory().empty(),"Source default state changed");
    for(int i=0;i<6;++i){stage.Input(nullptr,WM_KEYDOWN,'1'+i,0);Require(static_cast<int>(stage.GetActiveTab())==i,"Keyboard 1..6 did not select all existing tabs");stage.AdvanceTabPresentation(.18);}
    Tab(stage,EMaintenanceTab::Party);
    auto text=RenderText(stage,canvas);Has(text,L"ACTIVE PARTY","Party title missing");Has(text,L"5 / 5","Active party count missing");
    for(auto* c:game.GetRoster())Has(text,c->GetName(),"A roster name was clipped or omitted");
    Shot(stage,canvas,output,"maintenance-party");
    RosterClick(stage,15);Require(game.GetParty().size()==5,"Party maximum guard failed");
    Has(RenderText(stage,canvas),L"Active party is already full.","Party maximum explanation changed");
    for(int id:{2,8,11,14})RosterClick(stage,id);
    Require(game.GetParty().size()==1,"Party remove did not preserve remaining member");
    RosterClick(stage,1);Require(game.GetParty().size()==1,"Last party member removal was allowed");
    Has(RenderText(stage,canvas),L"At least one active character is required.","Last-member explanation changed");
    for(int id:{2,8,11,14})RosterClick(stage,id);
    Require(game.GetParty().size()==5,"Party restore through actual targets failed");

    Tab(stage,EMaintenanceTab::Inventory);
    Has(RenderText(stage,canvas),L"No Items Found.","Empty inventory text lost");
    Tab(stage,EMaintenanceTab::Shop);
    for(int tier=1;tier<=3;++tier){game.MaxUnlockedBattleStageId=tier;const auto catalog=stage.GetShopItemIDs();Require(catalog.size()==(tier==1?4:tier==2?8:10),"Actual shop tier stock changed");}
    game.MaxUnlockedBattleStageId=3;
    const auto catalog=stage.GetShopItemIDs();
    Click(stage,stage.GetShopSlotRect(0));
    Require(game.GetGold()==0&&game.GetInventory().empty(),"Insufficient-gold purchase changed inventory");
    Has(RenderText(stage,canvas),L"Cannot buy Small Potion. Not enough gold or invalid item.","Insufficient-gold purchase explanation changed");
    for(size_t i=0;i<catalog.size();++i){
        const int id=catalog[i],price=ItemData::GetPrice(id);game.SetGold(price);
        const int count=game.GetItemCount(id);Click(stage,stage.GetShopSlotRect(static_cast<int>(i)));
        Require(game.GetGold()==0&&game.GetItemCount(id)==count+1,"Actual purchase failed to spend exact price/add one");
        Has(RenderText(stage,canvas),L"Purchased "+ItemData::GetName(id)+L" for "+std::to_wstring(price)+L"G.","Purchase message changed");
    }
    game.SetGold(275);text=RenderText(stage,canvas);
    for(int id:catalog){Has(text,ItemData::GetName(id),"Shop name omitted");Has(text,std::to_wstring(ItemData::GetPrice(id))+L"G","Shop price omitted");}
    Shot(stage,canvas,output,"maintenance-shop");
    Tab(stage,EMaintenanceTab::Inventory);Click(stage,stage.GetPartySelectRect(0));
    auto* target=game.GetParty()[0];
    text=RenderText(stage,canvas);Has(text,target->GetName(),"Inventory target name omitted");Has(text,L"275","Inventory gold missing");
    for(int id:catalog)Has(text,ItemData::GetName(id),"Inventory item omitted");
    Shot(stage,canvas,output,"maintenance-inventory");
    auto stats=target->GetStat();stats.CurHP=20;stats.CurMP=0;target->SetStat(stats);
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(101)));
    Require(target->GetStat().CurHP==70&&game.GetItemCount(101)==0,"Actual HP potion did not heal/consume");
    Click(stage,stage.GetInventorySlotRect(InventoryIndex(102)));
    Require(target->GetStat().CurMP==(std::min)(30,target->GetStat().MaxMP)&&game.GetItemCount(102)==0,"Actual MP potion did not heal/consume");
    const auto base=target->GetStat();
    for(int id:{201,302,402}){Tab(stage,EMaintenanceTab::Inventory);Click(stage,stage.GetInventorySlotRect(InventoryIndex(id)));Require(stage.GetActiveTab()==EMaintenanceTab::Equipment,"Equip item did not open equipment tab");Require(game.GetEquippedItemID(target,ItemData::GetEquipPart(id))==id&&game.GetItemCount(id)==0,"Actual equipment application failed");stage.AdvanceTabPresentation(.18);}
    Require(target->GetStat().STR==base.STR+5&&target->GetStat().SPD==base.SPD+2&&target->GetStat().DEF==base.DEF+6,"Equipment stat/negative bonus lost");
    text=RenderText(stage,canvas);
    for(const wchar_t* label:{L"HP",L"MP",L"STR",L"INT",L"DEF",L"SPD",L"Weapon",L"Armor",L"Accessory"})Has(text,label,"Equipment visible stat/slot omitted");
    for(int id:{201,302,402})Has(text,ItemData::GetName(id),"Equipped item name omitted");
    Shot(stage,canvas,output,"maintenance-equipment");
    for(int i=0;i<3;++i)Click(stage,stage.GetEquipmentSlotRect(i));
    Require(target->GetEquipment().WeaponID==-1&&target->GetEquipment().ArmorID==-1&&target->GetEquipment().AccessoryID==-1,"Unequip did not clear all three parts");
    Require(game.GetItemCount(201)==1&&game.GetItemCount(302)==1&&game.GetItemCount(402)==1,"Unequip did not return exact inventory items");
    Require(target->GetStat().STR==base.STR&&target->GetStat().SPD==base.SPD&&target->GetStat().DEF==base.DEF,"Unequip did not restore stat bonuses");

    std::set<int> exercised;
    for(auto* character:game.GetRoster()){
        const int id=character->GetCharacterID();
        if(PartyIndex(id)<0){RosterClick(stage,game.GetParty().back()->GetCharacterID());RosterClick(stage,id);}
        Tab(stage,EMaintenanceTab::Skill);Click(stage,stage.GetSkillSlotRect(1));
        const int retainedSlot=stage.GetSelectedSkillSlot();Click(stage,stage.GetPartySelectRect(PartyIndex(id)));
        Require(game.GetParty()[stage.GetSelectedPartyIndex()]==character&&stage.GetSelectedSkillSlot()==retainedSlot,"Character switch lost selected skill slot");
        stage.AdvanceTabPresentation(.16);
        const auto learned=character->GetLearnedSkillIDs();Require(learned.size()==3,"Class/element learned skill set changed");
        text=RenderText(stage,canvas);Has(text,character->GetName(),"Skill character name missing");Has(text,character->GetElementName(),"Skill character element missing");
        Has(text,L"Equipped Skills","Equipped skills heading missing");Has(text,L"Learned Skills","Learned skills guide missing");
        for(int i=0;i<3;++i){
            const auto* skill=SkillData::Find(learned[i]);Require(skill&&SkillData::CanCharacterUse(character,learned[i]),"Source learned skill class/element mismatch");
            Has(text,skill->Name,"A learned skill name was clipped/omitted");Has(text,L"MP "+std::to_wstring(skill->MPCost),"Actual skill MP cost not rendered");Has(text,L"R"+std::to_wstring(skill->Range),"Actual skill range not rendered");
            Click(stage,stage.GetSkillSlotRect(i));Click(stage,stage.GetLearnedSkillRect(i));
            Require(character->GetEquippedSkillID(i)==learned[i],"Actual skill-card click did not equip correct slot");
            Has(RenderText(stage,canvas),L"Equipped "+skill->Name+L" to slot "+std::to_wstring(i+1)+L".","Exact skill equip feedback missing");
            exercised.insert(learned[i]);
        }
        const int mp=character->GetStat().CurMP,gold=game.GetGold(),last=character->GetEquippedSkillID(2);
        Click(stage,stage.GetSkillSlotRect(1));Click(stage,stage.GetLearnedSkillRect(0));
        Require(character->GetEquippedSkillID(0)==learned[0]&&character->GetEquippedSkillID(1)==learned[0]&&character->GetEquippedSkillID(2)==last,"Duplicate skill assignment was incorrectly swapped/rejected");
        Require(character->GetStat().CurMP==mp&&game.GetGold()==gold,"Maintenance skill equip spent combat resources");
    }
    Require(exercised.size()==45,"Not all 45 production skills were exercised");
    Shot(stage,canvas,output,"maintenance-skills");
    const std::wstring lastSkillNotice=L"Equipped "+SkillData::GetName(game.GetParty()[stage.GetSelectedPartyIndex()]->GetLearnedSkillIDs()[0])+L" to slot 2.";
    Has(RenderText(stage,canvas),lastSkillNotice,"Skill status should be visible before its legacy lifetime ends");
    for(int frame=0;frame<180;++frame)stage.Update(nullptr);
    Require(RenderText(stage,canvas).find(lastSkillNotice)==std::wstring::npos,"Skill status lifetime does not expire through public updates");
    Tab(stage,EMaintenanceTab::Reward);Has(RenderText(stage,canvas),L"Town mode: no recent battle reward.","Town reward empty state missing");
    Shot(stage,canvas,output,"maintenance-reward-empty");
    Require(stage.Input(nullptr,WM_KEYDOWN,VK_ESCAPE,0)==MAP_STAGE,"Maintenance Escape does not return to map");
}
void CheckRewards(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();auto& reward=BattleRewardManager::Get();Canvas canvas(client.right,client.bottom);
    for(const auto pair:std::array<std::pair<int,int>,4>{{{3,4},{4,5},{5,0},{5,6}}}){
        game.Clear();game.InitDefaultParty();reward.Clear();game.MaxUnlockedBattleStageId=pair.first;
        reward.SetDefaultVictoryReward(pair.first,pair.second);const int gold=reward.GetRewardGold();const auto items=reward.GetRewardItems();
        OffscreenMaintenanceStage stage;PrepareMaintenance(stage,client);
        stage.Update(nullptr);
        Require(game.GetGold()==gold&&game.GetParty().size()==5,"Actual victory reward did not apply to the default five-member party");
        for(const auto& item:items)Require(game.GetItemCount(item.ItemID)==item.Count,"Reward item count differs");
        std::vector<std::pair<int,int>> progression;for(auto* c:game.GetParty())progression.push_back({c->GetLevel(),c->GetExp()});
        stage.Update(nullptr);stage.Update(nullptr);
        Require(game.GetGold()==gold,"Repeated reward application added gold");
        for(size_t i=0;i<game.GetParty().size();++i)Require(progression[i]==std::make_pair(game.GetParty()[i]->GetLevel(),game.GetParty()[i]->GetExp()),"Repeated reward application added EXP");
        for(const auto& item:items)Require(game.GetItemCount(item.ItemID)==item.Count,"Repeated reward application duplicated items");
        const auto text=RenderText(stage,canvas);for(auto* c:game.GetParty())Has(text,c->GetName(),"Reward results omitted a party member");
        Has(text,L"Gold +"+std::to_wstring(gold),"Reward gold text missing");
        Has(text,L"EXP +"+std::to_wstring(reward.GetRewardExp()),"Reward EXP text missing");
        if(pair.second>=1&&pair.second<=5){
            Has(text,L"다음 스테이지","Valid next-stage control missing");
            Require(game.IsBattleStageUnlocked(pair.second),"Reward did not unlock next stage");
            Shot(stage,canvas,output,"maintenance-reward-next"+std::to_string(pair.second));
            Require(Click(stage,stage.GetNextStageRect())==BATTLE_STAGE&&game.GetCurrentBattleStageId()==pair.second,"Stage 3->4 or 4->5 reward navigation failed");
            Require(!reward.HasReward()&&game.GetGold()==gold,"Reward next navigation retained/reapplied reward");
        }else{
            Require(text.find(L"다음 스테이지")==std::wstring::npos,"Invalid/terminal next stage exposed an enter button");
            Require(Click(stage,stage.GetNextStageRect())!=BATTLE_STAGE,"Invalid/terminal next stage entered battle");
            Require(stage.Input(nullptr,WM_KEYDOWN,VK_ESCAPE,0)==MAP_STAGE,"Terminal reward Escape failed");
            Require(!reward.HasReward()&&game.GetGold()==gold,"Terminal reward exit changed payment");
        }
    }
    game.Clear();game.InitDefaultParty();reward.SetReward(false,2,0,999,999,{{101,9}});
    std::vector<std::pair<int,int>> defeatProgression;for(auto* actor:game.GetParty())defeatProgression.push_back({actor->GetLevel(),actor->GetExp()});
    OffscreenMaintenanceStage defeat;PrepareMaintenance(defeat,client);defeat.Update(nullptr);
    Require(game.GetGold()==0&&game.GetInventory().empty(),"Defeat incorrectly awarded victory rewards");
    for(size_t i=0;i<game.GetParty().size();++i)Require(defeatProgression[i]==std::make_pair(game.GetParty()[i]->GetLevel(),game.GetParty()[i]->GetExp()),"Defeat incorrectly awarded experience or levels");
    reward.Clear();
}
void CheckAlphaResource(int id,int expectedWidth=0,int expectedHeight=0)
{
    HINSTANCE module=GetModuleHandleW(nullptr);
    HRSRC found=FindResourceW(module,MAKEINTRESOURCEW(id),RT_BITMAP);Require(found!=nullptr,"New map artwork resource missing");
    const DWORD bytes=SizeofResource(module,found);const BYTE* raw=static_cast<const BYTE*>(LockResource(LoadResource(module,found)));
    Require(raw&&bytes>=sizeof(BITMAPINFOHEADER),"Invalid RT_BITMAP bytes");
    BITMAPINFOHEADER info{};std::memcpy(&info,raw,sizeof(info));
    const int height=std::abs(info.biHeight);
    Require(info.biSize==40&&info.biBitCount==32&&info.biCompression==BI_RGB&&info.biPlanes==1,"Artwork is not native 32-bit BGRA DIB");
    if(expectedWidth)Require(info.biWidth==expectedWidth&&height==expectedHeight,"Native crop dimensions differ from fixed world bounds");
    Require(bytes>=40+static_cast<size_t>(info.biWidth)*height*4,"Artwork resource is truncated");
    bool transparent=false,opaque=false,premultiplied=true;
    for(size_t i=0;i<static_cast<size_t>(info.biWidth)*height;++i){const BYTE* pixel=raw+40+i*4;const BYTE alpha=pixel[3];premultiplied&=pixel[0]<=alpha&&pixel[1]<=alpha&&pixel[2]<=alpha;transparent|=alpha==0;opaque|=alpha>0;}
    Require(premultiplied,"Resource RGB is not premultiplied");
    Require(transparent&&opaque,"Artwork lacks a transparent surround or visible subject");
}
void CheckMap(RECT client,const std::filesystem::path& output)
{
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    Map_Stage map;map.rects=client;map.Camera=new ACamera(0,0,0,0);
    map.Camera->currentZoom=map.Camera->targetZoom=static_cast<float>(client.right)/1672.0f;
    map.hBitBackground=LoadBitmapW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(121));
    Require(map.hBitBackground!=nullptr,"Clean native background resource missing");
    map.hasLayeredBackground=true;
    map.hBitMapOverview=LoadBitmapW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(IDB_BITMAP3));
    Require(map.hBitMapOverview!=nullptr,"Region popup overview resource missing");
    GetObject(map.hBitBackground,sizeof(BITMAP),&map.BackgroundInfo);
    map.SetupLandmarks(GetModuleHandleW(nullptr));
    Require(map.Markers.size()==6&&map.MarkerBattleStageIds==std::vector<int>({0,1,2,3,4,5}),"Actual map destinations changed");
    map.infoPopup=new URegionInfoPopup();map.infoPopup->Setup(GetModuleHandleW(nullptr),client);
    map.mainMenu=new UMainMenuPopup();map.mainMenu->Setup(GetModuleHandleW(nullptr),client);
    map.mainMenu->SetTeamData(game.GetParty());
    map.noticePopup=new UNoticePopup();map.noticePopup->Setup(client);
    map.screenUI.AddChildWindow(map.infoPopup);
    map.screenUI.AddChildWindow(map.mainMenu);
    Canvas canvas(client.right,client.bottom);
    map.Draw(canvas.dc,canvas.scratch);map.DrawCursor(canvas.dc,canvas.scratch);canvas.Save(output,"world-map");
    for(size_t i=0;i<map.Markers.size();++i){
        auto* marker=map.Markers[i];const RECT bounds=marker->GetWorldBounds();
        marker->SetPresentationActive(true);
        Require(marker->HasArtwork(),"Production landmark artwork did not load");
        Require(bounds.right>bounds.left&&bounds.bottom>bounds.top,"Marker fixed bounds invalid");
        if(client.right==1280)CheckAlphaResource(113+static_cast<int>(i),bounds.right-bounds.left,bounds.bottom-bounds.top);
        for(const float zoom:{0.75f,1.0f,1.8f}){
            const int ox=91,oy=57;const auto center=Center(bounds);
            const int x=static_cast<int>((center.x-ox)*zoom),y=static_cast<int>((center.y-oy)*zoom);
            Require(marker->ContainsPointWithOffset(x,y,ox,oy,zoom),"Offset/zoom building hit failed");
            Require(!marker->ContainsPointWithOffset(x,static_cast<int>((bounds.top-5-oy)*zoom),ox,oy,zoom),"Space above building/marker must not become input");
            marker->SetIsPointed(true);marker->AdvanceAnimation(.18);
            Require(std::abs(marker->GetVisualScale()-1.05f)<.001f,"Hover did not reach approved 1.05 scale");
            const RECT floating=marker->GetFloatingMarkerScreenRect(ox,oy,zoom);
            const RECT artwork=marker->GetBuildingScreenRect(ox,oy,zoom);
            const POINT floatingCenter=Center(floating);
            Require(floating.bottom<=artwork.top,"Floating sign overlaps enlarged building");
            Require(!marker->ContainsPointWithOffset(floatingCenter.x,floatingCenter.y,ox,oy,zoom),"Floating sign became a clickable input target");
            Require(marker->ContainsPointWithOffset(x,y,ox,oy,zoom),"Hover moved fixed hit area");
            Require(!marker->ContainsPointWithOffset(x,static_cast<int>((bounds.top-5-oy)*zoom),ox,oy,zoom),"Expanded roof altered input area");
            marker->SetIsPointed(false);marker->Deactivate();marker->AdvanceAnimation(.18);
            Require(std::abs(marker->GetVisualScale()-1.0f)<.001f,"Hover leave did not restore base scale");
        }
        map.Camera->g_CamX=map.Camera->g_CamY=0;
        map.Camera->currentZoom=map.Camera->targetZoom=static_cast<float>(client.right)/1672.0f;
        const auto c=Center(bounds);const float z=map.Camera->GetZoom();
        map.Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(static_cast<int>(c.x*z),static_cast<int>(c.y*z)));
        if(i>=4){Require(map.noticePopup->IsOpened()&&!map.infoPopup->GetVisible(),"Locked building did not block entry");map.noticePopup->CloseNotice();}
        else {
            Require(map.infoPopup->GetVisible()&&map.infoPopup->rName==marker->GetName()&&map.infoPopup->rEnv==marker->GetEnv()&&map.infoPopup->rDesc==marker->GetDesc(),"Actual region click lost name/environment/description");
            Require(map.SelectedMarkerNextStageID==(i==0?MAINTENANCE_STAGE:BATTLE_STAGE),"Map destination route differs");
            map.infoPopup->AdvancePresentation(.20);
            if(i==1){map.Draw(canvas.dc,canvas.scratch);map.DrawCursor(canvas.dc,canvas.scratch);canvas.Save(output,"world-map-region");}
            map.Input(nullptr,WM_KEYDOWN,VK_ESCAPE,0);Require(!map.infoPopup->GetVisible(),"Map region Escape failed");
            Require(map.infoPopup->IsClosing()&&map.infoPopup->IsPresenting(),"Region Escape skipped its closing presentation");
            const auto cameraPoint=map.Camera->GetPointGrid();
            const int selectedStage=map.SelectedMarkerNextStageID;
            const int itemCount=game.GetItemCount(101);
            map.stageItems.push_back(Map_Stage::SMapStageItem(999,101,30,30));
            const auto itemPoint=MAKELPARAM(static_cast<int>(30*z),static_cast<int>(30*z));
            Require(map.Input(nullptr,WM_LBUTTONDOWN,0,itemPoint)==-2,"Closing popup allowed a stage request");
            map.Input(nullptr,WM_LBUTTONUP,0,itemPoint);
            const auto other=Center(map.Markers[(i+1)%map.Markers.size()]->GetWorldBounds());
            map.Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(static_cast<int>(other.x*z),static_cast<int>(other.y*z)));
            map.Input(nullptr,WM_LBUTTONUP,0,MAKELPARAM(static_cast<int>(other.x*z),static_cast<int>(other.y*z)));
            map.Input(nullptr,WM_KEYDOWN,'I',0);map.Input(nullptr,WM_KEYDOWN,'P',0);map.Input(nullptr,WM_KEYDOWN,VK_ESCAPE,0);
            Require(game.GetItemCount(101)==itemCount&&map.stageItems.size()==1&&map.stageItems.front().Active,"Closing popup leaked a map-item pickup");
            Require(map.Camera->GetPointGrid()==cameraPoint&&map.SelectedMarkerNextStageID==selectedStage,"Closing popup leaked camera/region selection");
            Require(!map.mainMenu->GetVisible()&&!map.infoPopup->GetVisible()&&!map.noticePopup->IsOpened(),"Rapid closing input reopened another modal");
            map.infoPopup->AdvancePresentation(.08);
            Require(map.infoPopup->IsClosing()&&map.infoPopup->IsInputBlocked(),"Half-closed popup released modal input early");
            map.infoPopup->AdvancePresentation(.08);
            Require(!map.infoPopup->IsPresenting(),"Finished closing popup remains modal");
            map.stageItems.clear();
        }
    }
    // A committed ENTER can return one route only, even if duplicate messages arrive.
    map.Camera->g_CamX=map.Camera->g_CamY=0;
    map.Camera->currentZoom=map.Camera->targetZoom=static_cast<float>(client.right)/1672.0f;
    const auto camp=Center(map.Markers[0]->GetWorldBounds());const float campZoom=map.Camera->GetZoom();
    map.Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(static_cast<int>(camp.x*campZoom),static_cast<int>(camp.y*campZoom)));
    map.infoPopup->AdvancePresentation(.20);
    int entryCalls=0;
    map.infoPopup->SetOnEnterClick([&](){++entryCalls;map.Comp_NextStage(map.SelectedMarkerNextStageID);});
    const auto enter=map.infoPopup->btnEnter->GetLocation();const LPARAM enterPoint=MAKELPARAM(enter.x,enter.y);
    Require(map.Input(nullptr,WM_LBUTTONUP,0,enterPoint)==-2&&entryCalls==0,"Unpaired release entered a stage");
    Require(map.Input(nullptr,WM_LBUTTONDOWN,0,enterPoint)==-2,"ENTER down entered before release");
    Require(map.Input(nullptr,WM_LBUTTONUP,0,enterPoint)==MAINTENANCE_STAGE&&entryCalls==1,"CAMP ENTER did not return its original maintenance route exactly once");
    for(int repeat=0;repeat<4;++repeat){
        Require(map.Input(nullptr,WM_LBUTTONDOWN,0,enterPoint)==-2,"Committed ENTER accepted another press route");
        Require(map.Input(nullptr,WM_LBUTTONUP,0,enterPoint)==-2,"Committed ENTER returned a duplicate route");
    }
    Require(entryCalls==1&&map.infoPopup->IsInputBlocked(),"Repeated ENTER invoked the callback multiple times");
    if(client.right==1280){CheckAlphaResource(119,96,96);CheckAlphaResource(120,96,96);}
}
}
int wmain(int argc,wchar_t** argv)
{
    try{
        Require(argc==3,"Usage: maintenance_map_ui_checks <game-source> <output>");
        const auto source=std::filesystem::absolute(argv[1]),output=std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output);std::filesystem::current_path(source);
        CheckUIFrameTransitionBoundaries();
        for(const RECT client:{RECT{0,0,1280,720},RECT{0,0,1600,900},RECT{0,0,1264,644}}){
            CheckTabPresentation(client,output);CheckCharacterPresentation(client,output);CheckActionFeedback(client,output);CheckShopFeedback(client,output);CheckInventoryPotionFeedback(client,output);CheckPartyFeedback(client,output);CheckContinuousMaintenanceFlow(client,output);CheckMaintenance(client,output);CheckRewards(client,output);CheckMap(client,output);
        }
        std::ofstream report(output/"interaction-checks.json");
        report<<"{\n  \"passed\": true,\n  \"assertions\": "<<checks<<",\n  \"snapshots\": "<<snapshots<<",\n  \"clientSizes\": [[1280,720],[1600,900],[1264,644]],\n  \"scope\": \"Production stage Input/Draw, tab and character presentation start/mid/end, rapid retarget and character-to-tab continuity, fixed header/selector pixels, guarded and completed body actions, exact selected character and retained skill slot, real GameManager and reward singleton fixtures, 45 class-element skills, GDI text records and bitmap captures, raw alpha resources. No stage Init, game window, sound playback, or save file.\"\n}\n";
        std::cout<<"PASS: "<<checks<<" actual maintenance/map assertions; "<<snapshots<<" native captures.\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
