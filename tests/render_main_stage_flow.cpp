// Offscreen replay using the linked production stage Input/Draw objects.
// Reuse the tested memory canvas/setup helpers; the original test entry is never called.
#define wmain maintenance_checks_unused_entry
#include "maintenance_map_ui_checks.cpp"
#undef wmain
#include <psapi.h>
#include <iomanip>
#include "StageTransition.h"

namespace {
constexpr int NativeW=1280, NativeH=720, OutputW=800, OutputH=450, FPS=30, FrameCount=450;
const RECT Client{0,0,NativeW,NativeH};
constexpr double Step=1.0/60.0;
struct Keyframe { int index; const char* label; const char* file; };
const Keyframe Keyframes[]={{0,"map-idle","map-idle"},{60,"building-hover","building-hover"},
    {72,"region-opening-start","region-opening-start"},{74,"region-opening-mid","region-opening-mid"},
    {100,"region-open","region-open"},{135,"region-closing-start","region-closing-start"},
    {137,"region-closing-mid","region-closing-mid"},{150,"region-closed","region-closed"},
    {174,"region-reopening","region-reopening"},{237,"enter-pressed","enter-pressed"},
    {240,"maintenance-fade-start","maintenance-fade-start"},{244,"maintenance-fade-mid","maintenance-fade-mid"},
    {255,"maintenance-party","maintenance-party"},{309,"maintenance-equipment","maintenance-equipment"},
    {345,"maintenance-skills","maintenance-skills"},{381,"maintenance-shop","maintenance-shop"},
    {424,"map-return-mid","map-return-mid"},{449,"map-return","map-return"}};

std::unique_ptr<Map_Stage> PrepareMap(bool settleStartup=true)
{
    auto map=std::make_unique<Map_Stage>();const HINSTANCE module=GetModuleHandleW(nullptr);map->rects=Client;
    map->hBitBackground=LoadBitmapW(module,MAKEINTRESOURCEW(IDB_MAP_CLEAN));
    map->hBitMapOverview=LoadBitmapW(module,MAKEINTRESOURCEW(IDB_BITMAP3));
    Require(map->hBitBackground&&map->hBitMapOverview,"Actual map resources missing");
    map->hasLayeredBackground=true;GetObject(map->hBitBackground,sizeof(BITMAP),&map->BackgroundInfo);
    map->hBitWave=static_cast<HBITMAP>(LoadImageW(nullptr,L"Source\\SMap\\background\\Wave.bmp",IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION));
    if(map->hBitWave)GetObject(map->hBitWave,sizeof(BITMAP),&map->WaveInfo);
    map->TILE_SIZE=64;map->W=map->BackgroundInfo.bmWidth/64;map->H=map->BackgroundInfo.bmHeight/64;
    // These are Map_Stage::Init's exact constructor inputs, including the map zoom.
    map->Camera=new ACamera(map->BackgroundInfo.bmWidth/2-NativeW/2,map->BackgroundInfo.bmHeight/2-NativeH/2,
        (map->BackgroundInfo.bmWidth/2)/64,(map->BackgroundInfo.bmHeight/2)/64,Map_Stage::OverviewZoom);
    Require(map->Camera->GetZoom()==Map_Stage::OverviewZoom,"Initial production camera zoom changed");
    map->SetupLandmarks(module);Require(map->Markers.size()==6,"Expected all production landmarks");
    map->infoPopup=new URegionInfoPopup();map->infoPopup->Setup(module,Client);
    Map_Stage* ptr=map.get();
    map->infoPopup->SetOnEnterClick([ptr](){ptr->Comp_NextStage(ptr->SelectedMarkerNextStageID);});
    map->infoPopup->SetOnCloseClick([ptr](){for(auto* marker:ptr->Markers)marker->Deactivate();ptr->Camera->SetZoom(Map_Stage::OverviewZoom);});
    map->mainMenu=new UMainMenuPopup();map->mainMenu->Setup(module,Client);map->mainMenu->SetTeamData(GameManager::Get().GetParty());
    map->locationUI=new ULocationIndicator();map->locationUI->Setup(Client);
    map->noticePopup=new UNoticePopup();map->noticePopup->Setup(Client);
    map->screenUI.AddChildWindow(map->infoPopup);map->screenUI.AddChildWindow(map->mainMenu);map->screenUI.AddChildWindow(map->locationUI);
    if(!GameManager::Get().IsMapItemCollected(1))map->stageItems.emplace_back(1,101,600,400);
    // Begin after ordinary startup settling; no custom camera transform is assigned.
    if(settleStartup)for(int i=0;i<180;++i)map->Camera->Update(map->W,map->H,64,NativeW,NativeH);
    return map;
}
POINT WorldPoint(Map_Stage& map,POINT world)
{
    const auto camera=map.Camera->Getg_Cam();const float zoom=map.Camera->GetZoom();
    return {static_cast<LONG>((world.x-static_cast<int>(camera.first))*zoom),static_cast<LONG>((world.y-static_cast<int>(camera.second))*zoom)};
}
POINT Mix(POINT a,POINT b,double fraction)
{
    fraction=(std::max)(0.0,(std::min)(1.0,fraction));const double smooth=fraction*fraction*(3.0-2.0*fraction);
    return {static_cast<LONG>(std::lround(a.x+(b.x-a.x)*smooth)),static_cast<LONG>(std::lround(a.y+(b.y-a.y)*smooth))};
}
void InjectMapPointer(Map_Stage& map,POINT point)
{
    // Real WM_MOUSEMOVE reaches screenUI. OS-dependent RefreshPointer is replaced
    // only by this scripted client-pointer fixture, using the same production hit tests.
    Require(map.Input(nullptr,WM_MOUSEMOVE,0,MAKELPARAM(point.x,point.y))==-2,"Mouse move unexpectedly changed stage");
    const bool active=!map.infoPopup->IsPresenting()&&!map.mainMenu->GetVisible()&&!map.noticePopup->IsOpened();
    const auto camera=map.Camera->Getg_Cam();const int ox=static_cast<int>(camera.first),oy=static_cast<int>(camera.second);const float zoom=map.Camera->GetZoom();
    URegionMarker* closest=nullptr;float nearest=275.0f/zoom;
    for(auto* marker:map.Markers){
        marker->SetPresentationActive(active);
        marker->SetIsPointed(active&&marker->ContainsPointWithOffset(point.x,point.y,ox,oy,zoom));
        if(!active)continue;
        const float dx=marker->GetLocation().x-(ox+point.x/zoom),dy=marker->GetLocation().y-(oy+point.y/zoom);
        const float distance=std::sqrt(dx*dx+dy*dy);
        if(marker->GetIsPointed()||distance<nearest){closest=marker;nearest=marker->GetIsPointed()?0.0f:distance;}
    }
    if(closest)map.locationUI->SetInfo(closest->GetParentName(),closest->GetName());
    map.locationUI->SetVisible(closest!=nullptr);
}
void AdvanceMap(Map_Stage& map,POINT point)
{
    // Use the popup's public deterministic clock instead of its wall-clock Update.
    map.infoPopup->AdvancePresentation(Step);
    map.mainMenu->Update();map.locationUI->Update();map.noticePopup->Update();
    map.Camera->Update(map.W,map.H,64,NativeW,NativeH);
    InjectMapPointer(map,point);
    for(size_t i=0;i<map.Markers.size();++i){map.Markers[i]->SetIsUnlocked(i==0||GameManager::Get().IsBattleStageUnlocked(map.MarkerBattleStageIds[i]));map.Markers[i]->AdvanceAnimation(Step);}
    if(map.hBitWave){map.waveOffset-=1.0f;if(map.waveOffset<=-map.WaveInfo.bmWidth)map.waveOffset+=map.WaveInfo.bmWidth;}
}
void AdvanceQuill(UMouseMark& quill,POINT point,double time)
{
    quill.SetMousePos(point);quill.SetVisible(true);
    quill.swaySeconds=std::fmod((std::max)(0.0,time-Step),4.8);
    quill.lastUpdate=std::chrono::steady_clock::now()-std::chrono::microseconds(16667);
    quill.Update();
}
void CopyRaw(Canvas& native,Canvas& outputCanvas,std::vector<BYTE>& bytes)
{
    const int saved=SaveDC(outputCanvas.dc);SetStretchBltMode(outputCanvas.dc,HALFTONE);SetBrushOrgEx(outputCanvas.dc,0,0,nullptr);
    Require(StretchBlt(outputCanvas.dc,0,0,OutputW,OutputH,native.dc,0,0,NativeW,NativeH,SRCCOPY)!=0,"Native frame downscale failed");RestoreDC(outputCanvas.dc,saved);GdiFlush();
    for(size_t i=0;i<static_cast<size_t>(OutputW)*OutputH;++i){const auto pixel=outputCanvas.pixels[i];bytes[i*3]=static_cast<BYTE>(pixel);bytes[i*3+1]=static_cast<BYTE>(pixel>>8);bytes[i*3+2]=static_cast<BYTE>(pixel>>16);}
}

void SelectMaintenanceTab(Maintenance_Stage& stage,EMaintenanceTab tab)
{
    const POINT point=Center(stage.GetTabRect(tab));
    Require(stage.Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(point.x,point.y))==-2,"Tab click must stay in maintenance");
    Require(stage.GetActiveTab()==tab,"Actual tab handler selected the wrong tab");
}

void AdvanceMaintenance(Maintenance_Stage& stage,POINT cursor)
{
    stage.Input(nullptr,WM_MOUSEMOVE,0,MAKELPARAM(cursor.x,cursor.y));
    // This no-reward sequence needs only the same production presentation clock.
    // Update(nullptr) also advances wall-clock time, so it is not mixed with dt.
    stage.AdvanceTabPresentation(Step);
}

int RenderMaintenanceTabs(const std::filesystem::path& out)
{
    constexpr int Count=390;
    struct Selection { int frame;EMaintenanceTab tab;const char* label; };
    const Selection selections[]={{45,EMaintenanceTab::Equipment,"equipment"},
        {105,EMaintenanceTab::Skill,"skills"},{165,EMaintenanceTab::Shop,"shop"},
        {225,EMaintenanceTab::Party,"party"},{276,EMaintenanceTab::Equipment,"rapid-equipment"},
        {279,EMaintenanceTab::Skill,"rapid-skills"},{282,EMaintenanceTab::Shop,"rapid-shop"},
        {324,EMaintenanceTab::Party,"party-return"}};
    const Keyframe keys[]={{0,"party","party"},{45,"equipment-start","equipment-start"},
        {47,"equipment-mid","equipment-mid"},{60,"equipment","equipment"},
        {107,"skills-mid","skills-mid"},{120,"skills","skills"},
        {167,"shop-mid","shop-mid"},{180,"shop","shop"},
        {227,"party-mid","party-mid"},{240,"party-return","party-return"},
        {276,"rapid-equipment","rapid-equipment"},{279,"rapid-skills","rapid-skills"},
        {282,"rapid-shop","rapid-shop"},{284,"rapid-shop-mid","rapid-shop-mid"},
        {291,"rapid-shop-settled","rapid-shop-settled"},{326,"final-party-mid","final-party-mid"},
        {340,"final-party","final-party"},{389,"loop-end","loop-end"}};
    const auto started=std::chrono::steady_clock::now();
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    Require(game.GetRoster().size()==15&&game.GetParty().size()==5&&game.GetGold()==0&&game.GetInventory().empty(),"Expected real default camp data");
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,Client);Tab(stage,EMaintenanceTab::Party);
    Canvas native(NativeW,NativeH),outputCanvas(OutputW,OutputH);std::vector<BYTE> bytes(OutputW*OutputH*3);
    UMouseMark quill;quill.SetFrame(GetModuleHandleW(nullptr),{IDB_UI_MOUSECUSOR});Require(quill.IsDrawable(),"Actual quill resource missing");
    // Establish the same outgoing body snapshot as a normal first WM_TIMER draw.
    stage.Draw(native.dc,native.scratch);
    std::ofstream raw(out/"frames.bgr",std::ios::binary),trace(out/"frame-trace.csv");Require(bool(raw)&&bool(trace),"Cannot open maintenance flow outputs");
    trace<<"index,time,scene,active_tab,cursor_x,cursor_y,tab_transitioning,tab_progress,request\n";
    const POINT initial{1120,640};POINT cursor=initial;int lastSelection=-1;int matchedRawPixels=0,requests=0,rapidRetargets=0,movingFrames=0;
    std::string scene="party";
    for(int frame=0;frame<Count;++frame){
        const double time=frame/double(FPS);bool request=false;
        // Move to each real tab rectangle, including the 100ms retarget sequence.
        POINT origin=lastSelection>=0?Center(stage.GetTabRect(selections[lastSelection].tab)):initial;
        const int nextSelection=lastSelection+1;
        if(nextSelection<static_cast<int>(std::size(selections))){
            const auto& next=selections[nextSelection];
            const int previousFrame=lastSelection>=0?selections[lastSelection].frame:0;
            const int movementStart=(std::max)(previousFrame,next.frame-12);
            cursor=frame<movementStart?origin:Mix(origin,Center(stage.GetTabRect(next.tab)),double(frame-movementStart)/(next.frame-movementStart));
            if(frame==next.frame){
                if(frame==279||frame==282){Require(stage.IsTabTransitioning(),"Fast tab retarget missed the unfinished presentation");++rapidRetargets;}
                SelectMaintenanceTab(stage,next.tab);++requests;++lastSelection;request=true;scene=next.label;
                Require(stage.IsTabTransitioning(),"Tab switch did not start the production body presentation");
            }
        }else cursor=time>=12.0?Mix(origin,initial,(time-12.0)/.6):origin;
        for(int tick=0;tick<2;++tick){AdvanceMaintenance(stage,cursor);AdvanceQuill(quill,cursor,time+(tick+1)*Step);}
        stage.Draw(native.dc,native.scratch);
        // The only cursor in the output is above the body compositor.
        quill.Draw(native.dc,native.scratch);CopyRaw(native,outputCanvas,bytes);
        raw.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());Require(bool(raw),"Maintenance raw frame write failed");
        if(stage.IsTabTransitioning())++movingFrames;
        for(const auto& key:keys)if(frame==key.index){
            native.Save(out,key.file);outputCanvas.Save(out,std::string(key.file)+"-output");
            for(size_t i=0;i<bytes.size()/3;++i){const auto pixel=outputCanvas.pixels[i];Require(bytes[i*3]==static_cast<BYTE>(pixel)&&bytes[i*3+1]==static_cast<BYTE>(pixel>>8)&&bytes[i*3+2]==static_cast<BYTE>(pixel>>16),"Maintenance raw RGB proof mismatch");}
            matchedRawPixels+=OutputW*OutputH;
        }
        trace<<frame<<','<<time<<','<<scene<<','<<static_cast<int>(stage.GetActiveTab())<<','<<cursor.x<<','<<cursor.y<<','<<stage.IsTabTransitioning()<<','<<stage.GetTabTransitionProgress()<<','<<request<<'\n';
    }
    raw.close();trace.close();
    Require(requests==8&&rapidRetargets==2&&movingFrames>0,"Missing normal or rapid tab transitions");
    Require(stage.GetActiveTab()==EMaintenanceTab::Party&&!stage.IsTabTransitioning(),"Final party view did not settle");
    Require(game.GetRoster().size()==15&&game.GetParty().size()==5&&game.GetGold()==0&&game.GetInventory().empty(),"Tab preview mutated real default data");
    Require(std::filesystem::file_size(out/"frames.bgr")==static_cast<uintmax_t>(Count)*OutputW*OutputH*3,"Maintenance raw length mismatch");
    PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    std::ofstream manifest(out/"frames.json");manifest<<std::setprecision(8);
    manifest<<"{\n\"format\":\"bgr24\",\"rawFile\":\"frames.bgr\",\"width\":800,\"height\":450,\"stride\":2400,\"fps\":30,\"frameCount\":390,\"durationSeconds\":13,\"nativeViewport\":[1280,720],\n";
    manifest<<"\"scope\":\"Actual linked C++ Maintenance_Stage Input/Draw and production tab compositor replay, not a live game-window recording. No stage Init, audio, OS pointer movement, saved-game access, invented data, or preview-only transitions.\",\n";
    manifest<<"\"simulation\":{\"updateHz\":60,\"tabAnimation\":\"production AdvanceTabPresentation at 1/60s; body-only 0.18s fade; header, tabs and cursor remain outside composition\",\"pointer\":\"scripted client coordinates delivered to actual Input; real tab rectangles\",\"quill\":\"actual UMouseMark Update/Draw, native40px, drawn once above the body\",\"defaultRoster\":15,\"defaultParty\":5,\"defaultGold\":0,\"defaultInventoryEntries\":0},\n";
    manifest<<"\"timeline\":[{\"start\":0,\"end\":1.5,\"scene\":\"party\"},{\"start\":1.5,\"end\":3.5,\"scene\":\"equipment\"},{\"start\":3.5,\"end\":5.5,\"scene\":\"skills\"},{\"start\":5.5,\"end\":7.5,\"scene\":\"shop\"},{\"start\":7.5,\"end\":9.2,\"scene\":\"party\"},{\"start\":9.2,\"end\":9.3,\"scene\":\"rapid equipment\"},{\"start\":9.3,\"end\":9.4,\"scene\":\"rapid skills\"},{\"start\":9.4,\"end\":10.8,\"scene\":\"rapid shop\"},{\"start\":10.8,\"end\":13,\"scene\":\"party return\"}],\n\"stillFrameIndices\":[0,60,120,180],\n\"keyframes\":[";
    for(size_t i=0;i<std::size(keys);++i){const auto& key=keys[i];if(i)manifest<<',';manifest<<"{\"index\":"<<key.index<<",\"label\":\""<<key.label<<"\",\"file\":\""<<key.file<<"-1280x720.bmp\",\"outputFile\":\""<<key.file<<"-output-800x450.bmp\"}";}
    manifest<<"],\n\"checks\":{\"passed\":true,\"assertions\":"<<checks<<",\"tabRequests\":"<<requests<<",\"rapidRetargets\":"<<rapidRetargets<<",\"transitionFrames\":"<<movingFrames<<",\"rawKeyframePixelsMatched\":"<<matchedRawPixels<<",\"defaultDataUnchanged\":true},\"renderWallSeconds\":"<<seconds<<",\"peakWorkingSetBytes\":"<<memory.PeakWorkingSetSize<<"\n}\n";
    Require(bool(manifest),"Could not write maintenance flow manifest");
    std::cout<<"PASS: "<<Count<<" actual maintenance frames, "<<requests<<" tab clicks, "<<rapidRetargets<<" rapid retargets, "<<seconds<<" seconds.\n";
    return 0;
}
}

int RenderMaintenanceCharacters(const std::filesystem::path& out)
{
    constexpr int Count=420;
    enum class Kind { Member,Tab,Slot };
    struct Action { int frame;Kind kind;int value;const char* label; };
    const Action actions[]={{42,Kind::Member,2,"equipment-archer"},{90,Kind::Member,3,"equipment-mage"},
        {138,Kind::Tab,4,"skills-mage"},{153,Kind::Slot,1,"skills-slot-two"},
        {174,Kind::Member,0,"skills-warrior"},{222,Kind::Member,2,"skills-archer"},{264,Kind::Member,3,"skills-mage"},
        {276,Kind::Member,2,"rapid-archer"},{279,Kind::Member,3,"rapid-mage"},{282,Kind::Member,0,"rapid-warrior"},
        {330,Kind::Member,3,"character-to-tab-mage"},{333,Kind::Tab,3,"equipment-return"},
        {339,Kind::Member,2,"equipment-archer-return"},{378,Kind::Member,0,"equipment-warrior-return"}};
    const Keyframe keys[]={{0,"equipment-warrior","equipment-warrior"},{44,"equipment-archer-mid","equipment-archer-mid"},
        {60,"equipment-archer","equipment-archer"},{92,"equipment-mage-mid","equipment-mage-mid"},
        {108,"equipment-mage","equipment-mage"},{140,"skills-tab-mid","skills-tab-mid"},
        {160,"skills-slot-two","skills-slot-two"},{176,"skills-warrior-mid","skills-warrior-mid"},
        {190,"skills-warrior","skills-warrior"},{240,"skills-archer","skills-archer"},
        {279,"rapid-mage","rapid-mage"},{282,"rapid-warrior","rapid-warrior"},{290,"rapid-warrior-settled","rapid-warrior-settled"},
        {332,"character-to-tab-mid","character-to-tab-mid"},{335,"equipment-return-mid","equipment-return-mid"},
        {341,"equipment-archer-return-mid","equipment-archer-return-mid"},{390,"equipment-warrior-final","equipment-warrior-final"},
        {419,"loop-end","loop-end"}};
    const auto started=std::chrono::steady_clock::now();
    auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
    const auto& party=game.GetParty();
    Require(party.size()==5&&party[0]->GetClassType()==ECharacterClass::Warrior&&party[2]->GetClassType()==ECharacterClass::Archer&&party[3]->GetClassType()==ECharacterClass::Mage,"Unexpected default party classes");
    std::vector<int> initialPartyIDs;std::vector<std::vector<int>> initialSkills;
    for(const auto* member:party){initialPartyIDs.push_back(member->GetCharacterID());initialSkills.push_back(member->GetEquippedSkillIDs());}
    OffscreenMaintenanceStage stage;PrepareMaintenance(stage,Client);Tab(stage,EMaintenanceTab::Equipment);
    Canvas native(NativeW,NativeH),outputCanvas(OutputW,OutputH);std::vector<BYTE> bytes(OutputW*OutputH*3);
    UMouseMark quill;quill.SetFrame(GetModuleHandleW(nullptr),{IDB_UI_MOUSECUSOR});Require(quill.IsDrawable(),"Actual quill resource missing");
    stage.Draw(native.dc,native.scratch);
    auto actionPoint=[&](const Action& action){
        if(action.kind==Kind::Member)return Center(stage.GetPartySelectRect(action.value));
        if(action.kind==Kind::Tab)return Center(stage.GetTabRect(static_cast<EMaintenanceTab>(action.value)));
        return Center(stage.GetSkillSlotRect(action.value));
    };
    std::ofstream raw(out/"frames.bgr",std::ios::binary),trace(out/"frame-trace.csv");Require(bool(raw)&&bool(trace),"Cannot open character preview outputs");
    trace<<"index,time,scene,active_tab,party_index,character_id,slot_index,cursor_x,cursor_y,tab_transitioning,tab_progress,character_transitioning,character_progress,request\n";
    const POINT initial{1120,640};POINT cursor=initial;int lastAction=-1,requests=0,rapidRetargets=0,crossRetargets=0,characterFrames=0,matchedRawPixels=0;
    std::string scene="equipment-warrior";
    for(int frame=0;frame<Count;++frame){
        const double time=frame/double(FPS);bool requested=false;
        const POINT origin=lastAction>=0?actionPoint(actions[lastAction]):initial;
        const int nextIndex=lastAction+1;
        if(nextIndex<static_cast<int>(std::size(actions))){
            const auto& next=actions[nextIndex];const int previousFrame=lastAction>=0?actions[lastAction].frame:0;
            const int movementStart=(std::max)(previousFrame,next.frame-12);
            cursor=frame<movementStart?origin:Mix(origin,actionPoint(next),double(frame-movementStart)/(next.frame-movementStart));
            if(frame==next.frame){
                if(frame==279||frame==282){Require(stage.IsCharacterTransitioning(),"Rapid character click missed the active blend");++rapidRetargets;}
                if(frame==333){Require(stage.IsCharacterTransitioning(),"Character-to-tab switch must interrupt an active character blend");++crossRetargets;}
                Require(stage.Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(cursor.x,cursor.y))==-2,"Character preview click left maintenance");
                if(next.kind==Kind::Member){Require(stage.GetSelectedPartyIndex()==next.value,"Actual character selection failed");Require(stage.IsCharacterTransitioning(),"Character selection did not start its production presentation");}
                if(next.kind==Kind::Tab)Require(static_cast<int>(stage.GetActiveTab())==next.value&&stage.IsTabTransitioning(),"Actual tab switch did not start");
                if(next.kind==Kind::Slot)Require(stage.GetSelectedSkillSlot()==1,"Actual second slot selection failed");
                ++requests;++lastAction;requested=true;scene=next.label;
            }
        }else cursor=time>=13.2?Mix(origin,initial,(time-13.2)/.6):origin;
        for(int tick=0;tick<2;++tick){AdvanceMaintenance(stage,cursor);AdvanceQuill(quill,cursor,time+(tick+1)*Step);}
        if(frame>=153)Require(stage.GetSelectedSkillSlot()==1,"Character or tab switching reset the selected skill slot");
        stage.Draw(native.dc,native.scratch);quill.Draw(native.dc,native.scratch);CopyRaw(native,outputCanvas,bytes);
        raw.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());Require(bool(raw),"Character raw frame write failed");
        if(stage.IsCharacterTransitioning())++characterFrames;
        for(const auto& key:keys)if(frame==key.index){
            native.Save(out,key.file);outputCanvas.Save(out,std::string(key.file)+"-output");
            for(size_t i=0;i<bytes.size()/3;++i){const auto pixel=outputCanvas.pixels[i];Require(bytes[i*3]==static_cast<BYTE>(pixel)&&bytes[i*3+1]==static_cast<BYTE>(pixel>>8)&&bytes[i*3+2]==static_cast<BYTE>(pixel>>16),"Character preview raw proof mismatch");}
            matchedRawPixels+=OutputW*OutputH;
        }
        trace<<frame<<','<<time<<','<<scene<<','<<static_cast<int>(stage.GetActiveTab())<<','<<stage.GetSelectedPartyIndex()<<','<<party[stage.GetSelectedPartyIndex()]->GetCharacterID()<<','<<stage.GetSelectedSkillSlot()<<','<<cursor.x<<','<<cursor.y<<','<<stage.IsTabTransitioning()<<','<<stage.GetTabTransitionProgress()<<','<<stage.IsCharacterTransitioning()<<','<<stage.GetCharacterTransitionProgress()<<','<<requested<<'\n';
    }
    raw.close();trace.close();
    Require(requests==14&&rapidRetargets==2&&crossRetargets==1&&characterFrames>0,"Character preview route incomplete");
    Require(stage.GetActiveTab()==EMaintenanceTab::Equipment&&stage.GetSelectedPartyIndex()==0&&!stage.IsCharacterTransitioning()&&!stage.IsTabTransitioning(),"Final equipment view did not settle");
    Require(game.GetRoster().size()==15&&party.size()==5&&game.GetGold()==0&&game.GetInventory().empty(),"Character preview mutated default data");
    for(size_t i=0;i<party.size();++i)Require(party[i]->GetCharacterID()==initialPartyIDs[i]&&party[i]->GetEquippedSkillIDs()==initialSkills[i],"Character preview changed party order or equipped skills");
    Require(std::filesystem::file_size(out/"frames.bgr")==static_cast<uintmax_t>(Count)*OutputW*OutputH*3,"Character raw length mismatch");
    PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
    std::ofstream manifest(out/"frames.json");manifest<<std::setprecision(8);
    manifest<<"{\n\"format\":\"bgr24\",\"rawFile\":\"frames.bgr\",\"width\":800,\"height\":450,\"stride\":2400,\"fps\":30,\"frameCount\":420,\"durationSeconds\":14,\"nativeViewport\":[1280,720],\n";
    manifest<<"\"scope\":\"Actual linked C++ Maintenance_Stage Input/Draw and production character/tab compositor replay, not a live game-window recording. No stage Init, audio, OS pointer movement, saved-game access, invented data, or preview-only transitions. Initial equipment tab uses the unchanged default party; slot selection changes only UI state.\",\n";
    manifest<<"\"simulation\":{\"updateHz\":60,\"characterAnimation\":\"production shared AdvanceTabPresentation at 1/60s; 0.16s character details fade\",\"tabAnimation\":\"production 0.18s body fade\",\"pointer\":\"scripted client coordinates delivered to actual Input and actual character/tab/slot rectangles\",\"quill\":\"actual UMouseMark Update/Draw, native40px, drawn once above composition\",\"partyIndices\":[0,2,3],\"selectedSkillSlot\":2,\"defaultGold\":0,\"defaultInventoryEntries\":0},\n\"timeline\":[{\"start\":0,\"end\":1.4,\"scene\":\"equipment-warrior\"}";
    for(size_t i=0;i<std::size(actions);++i){const auto& action=actions[i];manifest<<",{\"start\":"<<action.frame/double(FPS)<<",\"end\":"<<(i+1<std::size(actions)?actions[i+1].frame/double(FPS):14.0)<<",\"scene\":\""<<action.label<<"\"}";}
    manifest<<"],\n\"stillFrameIndices\":[0,60,108,240],\n\"keyframes\":[";
    for(size_t i=0;i<std::size(keys);++i){const auto& key=keys[i];if(i)manifest<<',';manifest<<"{\"index\":"<<key.index<<",\"label\":\""<<key.label<<"\",\"file\":\""<<key.file<<"-1280x720.bmp\",\"outputFile\":\""<<key.file<<"-output-800x450.bmp\"}";}
    manifest<<"],\n\"checks\":{\"passed\":true,\"assertions\":"<<checks<<",\"inputRequests\":"<<requests<<",\"rapidCharacterRetargets\":"<<rapidRetargets<<",\"characterToTabRetargets\":"<<crossRetargets<<",\"characterTransitionFrames\":"<<characterFrames<<",\"rawKeyframePixelsMatched\":"<<matchedRawPixels<<",\"skillSlotPreserved\":true,\"equippedSkillsUnchanged\":true,\"defaultDataUnchanged\":true},\"renderWallSeconds\":"<<seconds<<",\"peakWorkingSetBytes\":"<<memory.PeakWorkingSetSize<<"\n}\n";
    Require(bool(manifest),"Could not write character preview manifest");
    std::cout<<"PASS: "<<Count<<" actual maintenance character frames, "<<requests<<" clicks, "<<rapidRetargets<<" rapid retargets, "<<seconds<<" seconds.\n";
    return 0;
}

int wmain(int argc,wchar_t** argv)
{
    try{
        Require(argc==3||(argc==4&&(std::wstring(argv[3])==L"--maintenance-tabs"||std::wstring(argv[3])==L"--maintenance-characters")),"Usage: render_main_stage_flow <game-source> <output> [--maintenance-tabs|--maintenance-characters]");
        const auto source=std::filesystem::absolute(argv[1]),out=std::filesystem::absolute(argv[2]);std::filesystem::create_directories(out);std::filesystem::current_path(source);
        if(argc==4)return std::wstring(argv[3])==L"--maintenance-tabs"?RenderMaintenanceTabs(out):RenderMaintenanceCharacters(out);
        const auto started=std::chrono::steady_clock::now();auto& game=GameManager::Get();game.Clear();game.InitDefaultParty();BattleRewardManager::Get().Clear();
        auto map=PrepareMap();std::unique_ptr<OffscreenMaintenanceStage> maintenance;
        UMouseMark quill;quill.SetFrame(GetModuleHandleW(nullptr),{IDB_UI_MOUSECUSOR});Require(quill.IsDrawable(),"Actual40px cursor resource missing");
        Canvas native(NativeW,NativeH),outputCanvas(OutputW,OutputH);std::vector<BYTE> bytes(OutputW*OutputH*3);
        StageTransition transition;
        std::ofstream raw(out/"frames.bgr",std::ios::binary),trace(out/"frame-trace.csv");Require(bool(raw)&&bool(trace),"Cannot open frame outputs");
        trace<<"index,time,scene,cursor_x,cursor_y,zoom,cam_x,cam_y,camp_scale,popup_opacity,stage_progress\n";
        const POINT start{1120,640};POINT cursor=start,enter{},close{},equipment{},skill{},shop{},popupPointer{};
        bool inMaintenance=false,popupOpened=false,popupClosed=false,popupReopened=false,entered=false,returned=false;
        float hoverPeak=1.0f,leaveMinimum=1.05f;int matchedRawPixels=0;double cursorStageStart=0;
        for(int frame=0;frame<FrameCount;++frame){
            const double time=frame/double(FPS);std::string scene="map";
            if(frame<72){
                const POINT camp=WorldPoint(*map,Center(map->Markers[0]->GetWorldBounds()));
                if(time<.6)cursor=start;else if(time<1.4)cursor=Mix(start,camp,(time-.6)/.8);else cursor=camp;
            }
            if(frame==72||frame==174){
                cursor=WorldPoint(*map,Center(map->Markers[0]->GetWorldBounds()));
                Require(map->Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(cursor.x,cursor.y))==-2,"Camp down changed stage before popup");
                Require(map->Input(nullptr,WM_LBUTTONUP,0,MAKELPARAM(cursor.x,cursor.y))==-2,"Camp release changed stage before ENTER");
                Require(map->infoPopup->GetVisible()&&map->SelectedMarkerNextStageID==MAINTENANCE_STAGE,"Camp popup routing failed");
                const auto enterLoc=map->infoPopup->btnEnter->GetLocation();enter={enterLoc.x,enterLoc.y};
                const auto closeLoc=map->infoPopup->btnClose->GetLocation();close={closeLoc.x,closeLoc.y};
                popupPointer=cursor;
                if(frame==72)popupOpened=true;else popupReopened=true;
            }
            if(frame>=72&&frame<135){
                scene="region-popup";
                if(time>=3.9)cursor=Mix(popupPointer,close,(time-3.9)/.4);
            }
            if(frame==132){
                cursor=close;InjectMapPointer(*map,cursor);
                Require(map->Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(cursor.x,cursor.y))==-2,"CLOSE down must only press");
                Require(map->infoPopup->btnClose->GetIsPressed(),"Actual CLOSE did not press");
            }
            if(frame==135){
                cursor=close;
                Require(map->Input(nullptr,WM_LBUTTONUP,0,MAKELPARAM(cursor.x,cursor.y))==-2,"CLOSE must stay on the map");
                Require(!map->infoPopup->GetVisible()&&map->infoPopup->IsPresenting(),"CLOSE must retain a fading presentation after logical close");
                popupClosed=true;
            }
            if(frame>=135&&frame<174){
                scene="region-closes";
                if(time>=5.2)cursor=Mix(close,WorldPoint(*map,Center(map->Markers[0]->GetWorldBounds())),(time-5.2)/.4);
            }
            if(frame>=174&&frame<240){
                scene="region-reopened";
                if(time>=7.3)cursor=Mix(popupPointer,enter,(time-7.3)/.4);
            }
            if(frame==237){
                cursor=enter;InjectMapPointer(*map,cursor);
                Require(map->Input(nullptr,WM_LBUTTONDOWN,0,MAKELPARAM(cursor.x,cursor.y))==-2,"ENTER down must only press");
                Require(map->infoPopup->btnEnter->GetIsPressed(),"Actual ENTER did not press");
            }
            if(frame==240){
                cursor=enter;Require(map->Input(nullptr,WM_LBUTTONUP,0,MAKELPARAM(cursor.x,cursor.y))==MAINTENANCE_STAGE,"Actual ENTER did not return MAINTENANCE_STAGE");
                // The same compositor as Framework captures the old scene without a cursor.
                map->Draw(native.dc,native.scratch);
                Require(transition.Capture(native.dc,NativeW,NativeH),"Could not capture outgoing map");
                map.reset();
                maintenance=std::make_unique<OffscreenMaintenanceStage>();PrepareMaintenance(*maintenance,Client);Tab(*maintenance,EMaintenanceTab::Party);
                Require(game.GetParty().size()==5&&game.GetGold()==0&&game.GetInventory().empty(),"Entering camp changed default game data");
                equipment=Center(maintenance->GetTabRect(EMaintenanceTab::Equipment));skill=Center(maintenance->GetTabRect(EMaintenanceTab::Skill));shop=Center(maintenance->GetTabRect(EMaintenanceTab::Shop));
                transition.Start();inMaintenance=true;entered=true;cursorStageStart=time;
            }
            if(inMaintenance){
                scene="maintenance-party";
                if(time>=9.6&&time<10.0)cursor=Mix(enter,equipment,(time-9.6)/.4);
                if(frame==300){Require(!transition.BlocksInput(WM_LBUTTONDOWN),"Equipment input unexpectedly blocked");cursor=equipment;SelectMaintenanceTab(*maintenance,EMaintenanceTab::Equipment);}
                if(frame>=300)scene="maintenance-equipment";
                if(time>=10.8&&time<11.2)cursor=Mix(equipment,skill,(time-10.8)/.4);
                if(frame==336){cursor=skill;SelectMaintenanceTab(*maintenance,EMaintenanceTab::Skill);}
                if(frame>=336)scene="maintenance-skills";
                if(time>=12.0&&time<12.4)cursor=Mix(skill,shop,(time-12.0)/.4);
                if(frame==372){cursor=shop;SelectMaintenanceTab(*maintenance,EMaintenanceTab::Shop);}
                if(frame>=372)scene="maintenance-shop";
            }
            if(frame==420){
                Require(maintenance->Input(nullptr,WM_KEYDOWN,VK_ESCAPE,0)==MAP_STAGE,"Actual maintenance Escape did not return MAP_STAGE");
                maintenance->Draw(native.dc,native.scratch);
                Require(transition.Capture(native.dc,NativeW,NativeH),"Could not capture outgoing maintenance");
                maintenance.reset();map=PrepareMap(false);transition.Start();
                inMaintenance=false;returned=true;cursorStageStart=time;scene="map-return";
            }
            if(frame>420){cursor=Mix(shop,start,(time-14.0)/.5);scene="map-return";}
            for(int tick=0;tick<2;++tick){
                if(inMaintenance)AdvanceMaintenance(*maintenance,cursor);
                else AdvanceMap(*map,cursor);
                transition.Advance(Step);
                AdvanceQuill(quill,cursor,time-cursorStageStart+(tick+1)*Step);
            }
            if(frame>=48&&frame<=70)hoverPeak=(std::max)(hoverPeak,map->Markers[0]->GetVisualScale());
            if(frame>=150&&frame<=155)leaveMinimum=(std::min)(leaveMinimum,map->Markers[0]->GetVisualScale());
            if(inMaintenance)maintenance->Draw(native.dc,native.scratch);else map->Draw(native.dc,native.scratch);
            transition.Composite(native.dc,NativeW,NativeH);
            quill.Draw(native.dc,native.scratch);CopyRaw(native,outputCanvas,bytes);
            raw.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());Require(bool(raw),"Raw frame write failed");
            for(const auto& key:Keyframes)if(frame==key.index){
                native.Save(out,key.file);outputCanvas.Save(out,std::string(key.file)+"-output");
                for(size_t i=0;i<bytes.size()/3;++i){const auto pixel=outputCanvas.pixels[i];Require(bytes[i*3]==static_cast<BYTE>(pixel)&&bytes[i*3+1]==static_cast<BYTE>(pixel>>8)&&bytes[i*3+2]==static_cast<BYTE>(pixel>>16),"Raw BGR differs from downscaled native keyframe");}
                matchedRawPixels+=OutputW*OutputH;
            }
            const auto camera=map?map->Camera->Getg_Cam():std::pair<float,float>{0.0f,0.0f};
            trace<<frame<<','<<time<<','<<scene<<','<<cursor.x<<','<<cursor.y<<','<<(map?map->Camera->GetZoom():0)<<','<<camera.first<<','<<camera.second<<','<<(map?map->Markers[0]->GetVisualScale():0)<<','<<(map?map->infoPopup->GetPresentationOpacity():0)<<','<<transition.Progress()<<'\n';
        }
        raw.close();trace.close();Require(popupOpened&&popupClosed&&popupReopened&&entered&&returned,"Required route missing");Require(hoverPeak>1.049f&&leaveMinimum<1.001f,"Hover enter/leave scale did not complete");
        Require(std::filesystem::file_size(out/"frames.bgr")==static_cast<uintmax_t>(FrameCount)*OutputW*OutputH*3,"Raw frame byte length wrong");
        Require(game.GetGold()==0&&game.GetInventory().empty()&&game.GetParty().size()==5,"Render sequence changed party/inventory/gold");
        PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory));
        const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
        std::ofstream manifest(out/"frames.json");manifest<<std::setprecision(8)<<"{\n\"format\":\"bgr24\",\"rawFile\":\"frames.bgr\",\"width\":"<<OutputW<<",\"height\":"<<OutputH<<",\"stride\":"<<OutputW*3<<",\"fps\":30,\"frameCount\":450,\"durationSeconds\":15,\"nativeViewport\":[1280,720],\n";
        manifest<<"\"scope\":\"Actual linked C++ Input/Draw and production popup/stage compositor replay, not a live game-window capture. No stage Init, audio, OS pointer movement or save access. No preview-only transitions.\",\n";
        manifest<<"\"simulation\":{\"cameraHz\":60,\"initialCameraConstructor\":[196,110,13,7],\"initialZoom\":"<<Map_Stage::OverviewZoom<<",\"startupCameraUpdates\":180,\"returnCameraStartupUpdates\":0,\"cameraSmoothingPerUpdate\":0.1,\"markerAnimation\":\"production AdvanceAnimation at 1/60s\",\"popupAnimation\":\"production AdvancePresentation at 1/60s, open0.20s close0.16s\",\"stageAnimation\":\"production StageTransition, 0.30s smoothstep, cursor drawn afterwards\",\"pointer\":\"scripted client coordinates: actual Input plus public production marker hit-test hover injection\",\"quill\":\"actual UMouseMark Update with lastUpdate=now-16667us; native40px nib2,37, new-stage phase reset\"},\n";
        manifest<<"\"timeline\":[{\"start\":0,\"end\":2.4,\"scene\":\"map and building hover\"},{\"start\":2.4,\"end\":4.5,\"scene\":\"region popup fades in\"},{\"start\":4.5,\"end\":5.8,\"scene\":\"CLOSE fades out and camera returns\"},{\"start\":5.8,\"end\":8.0,\"scene\":\"reopen region popup and ENTER press/release\"},{\"start\":8.0,\"end\":10.0,\"scene\":\"crossfade to maintenance party\"},{\"start\":10.0,\"end\":11.2,\"scene\":\"equipment\"},{\"start\":11.2,\"end\":12.4,\"scene\":\"skills\"},{\"start\":12.4,\"end\":14.0,\"scene\":\"shop\"},{\"start\":14.0,\"end\":15.0,\"scene\":\"Escape and crossfade back to map\"}],\n\"keyframes\":[";
        for(size_t i=0;i<std::size(Keyframes);++i){const auto& key=Keyframes[i];if(i)manifest<<',';manifest<<"{\"index\":"<<key.index<<",\"label\":\""<<key.label<<"\",\"file\":\""<<key.file<<"-1280x720.bmp\",\"outputFile\":\""<<key.file<<"-output-800x450.bmp\"}";}
        manifest<<"],\n\"checks\":{\"passed\":true,\"assertions\":"<<checks<<",\"rawKeyframePixelsMatched\":"<<matchedRawPixels<<",\"hoverPeak\":"<<hoverPeak<<",\"leaveMinimum\":"<<leaveMinimum<<"},\"renderWallSeconds\":"<<seconds<<",\"peakWorkingSetBytes\":"<<memory.PeakWorkingSetSize<<"\n}\n";
        std::cout<<"PASS: "<<FrameCount<<" production-rendered frames, "<<seconds<<" seconds, peak RAM "<<memory.PeakWorkingSetSize<<" bytes.\n";return 0;
    }catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
