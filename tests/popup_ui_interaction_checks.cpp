// Headless checks of production popup callbacks and memory-DC rendering.
// Build/run is controlled by the integration owner. No stage Init, window,
// gameplay loop, sound start, save call, or operating-system quit is invoked.
#include <windows.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#define private public
#define protected public
#include "UMainMenuPopup.h"
#include "USettingPopup.h"
#include "URegionInfoPopup.h"
#include "UNoticePopup.h"
#include "Map_Stage.h"
#include "Title_Stage.h"
#undef protected
#undef private

namespace {
int checks = 0;
void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
    ++checks;
}
UButton* Button(UContainerWidget* parent, const std::wstring& text)
{
    for (UWidget* child : parent->Contents) {
        if (auto button = dynamic_cast<UButton*>(child)) if (button->ButtonText == text) return button;
        if (auto container = dynamic_cast<UContainerWidget*>(child)) if (auto result = Button(container, text)) return result;
    }
    return nullptr;
}
void Click(UContainerWidget& surface, UButton* button)
{
    Require(button != nullptr, "Requested production button not found");
    const Location p = button->GetLocation();
    Require(surface.FindWidgetUnderMouse(p.x, p.y) == button, "Visible production button is not the hit target");
    button->Click();
}
UButton* ItemButton(const std::vector<UButton*>& buttons, int id)
{
    const auto name = ItemData::GetName(id);
    for (auto button : buttons)
        if (button->ButtonText == name || button->ButtonText.find(name + L" x") == 0) return button;
    throw std::runtime_error("Owned item button not found");
}
void Hover(UContainerWidget& surface, UButton* button)
{
    Require(button != nullptr, "Hover button missing");
    surface.UpdateMouseHover(-10000, -10000); surface.Update();
    const auto position = button->GetLocation();
    surface.UpdateMouseHover(position.x, position.y); surface.Update();
}
void SeedCatalog()
{
    auto& game = GameManager::Get();
    game.ClearInventory();
    for (const auto& item : ItemData::GetAllItems()) game.AddItem(item.ItemID, 3);
}
struct Canvas {
    int width, height;
    HDC dc = nullptr, scratch = nullptr;
    HBITMAP bitmap = nullptr;
    HGDIOBJ previous = nullptr;
    std::uint32_t* pixels = nullptr;
    Canvas(int w, int h) : width(w), height(h) {
        BITMAPINFO info{};
        info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth = width;
        info.bmiHeader.biHeight = -height;
        info.bmiHeader.biPlanes = 1;
        info.bmiHeader.biBitCount = 32;
        info.bmiHeader.biCompression = BI_RGB;
        dc = CreateCompatibleDC(nullptr);
        scratch = CreateCompatibleDC(nullptr);
        void* data = nullptr;
        bitmap = CreateDIBSection(nullptr, &info, DIB_RGB_COLORS, &data, nullptr, 0);
        Require(dc && scratch && bitmap && data, "Memory canvas creation failed");
        pixels = static_cast<std::uint32_t*>(data);
        previous = SelectObject(dc, bitmap);
    }
    ~Canvas() { if (dc && previous) SelectObject(dc, previous); if (bitmap) DeleteObject(bitmap); if (scratch) DeleteDC(scratch); if (dc) DeleteDC(dc); }
    void Background(HBITMAP map) {
        BITMAP info{};
        Require(GetObject(map, sizeof(info), &info) != 0, "Map bitmap unavailable");
        HGDIOBJ old = SelectObject(scratch, map);
        SetStretchBltMode(dc, COLORONCOLOR);
        Require(StretchBlt(dc, 0, 0, width, height, scratch, 0, 0, info.bmWidth, info.bmHeight, SRCCOPY) != 0, "Map drawing failed");
        SelectObject(scratch, old);
    }
    void Save(const std::filesystem::path& destination) {
        GdiFlush();
        // BI_RGB has no alpha contract; make the exported screenshot explicitly opaque.
        for (size_t i = 0; i < static_cast<size_t>(width) * height; ++i) pixels[i] |= 0xff000000u;
        BITMAPFILEHEADER file{};
        BITMAPINFOHEADER info{};
        info.biSize = sizeof(info); info.biWidth = width; info.biHeight = -height;
        info.biPlanes = 1; info.biBitCount = 32; info.biCompression = BI_RGB;
        info.biSizeImage = static_cast<DWORD>(width * height * 4);
        file.bfType = 0x4d42; file.bfOffBits = sizeof(file) + sizeof(info); file.bfSize = file.bfOffBits + info.biSizeImage;
        std::ofstream stream(destination, std::ios::binary);
        Require(bool(stream), "Cannot create popup snapshot");
        stream.write(reinterpret_cast<const char*>(&file), sizeof(file));
        stream.write(reinterpret_cast<const char*>(&info), sizeof(info));
        stream.write(reinterpret_cast<const char*>(pixels), info.biSizeImage);
        Require(bool(stream), "Cannot write popup snapshot");
    }
};
void Snapshot(Canvas& canvas, HBITMAP map, UWidget& widget, const std::filesystem::path& out, const std::string& name)
{
    canvas.Background(map);
    widget.Draw(canvas.dc, canvas.scratch);
    canvas.Save(out / (name + "-" + std::to_string(canvas.width) + ".bmp"));
}
void CheckButtonGuards()
{
    UButton button;
    int invoked = 0;
    button.SetOnClick([&]() { ++invoked; });
    button.SetVisible(false); button.Click();
    Require(invoked == 0, "Hidden button invoked callback");
    button.SetVisible(true); button.SetEnabled(false); button.Click();
    Require(invoked == 0, "Disabled button invoked callback");
    button.SetEnabled(true); button.Click();
    Require(invoked == 1, "Enabled visible button did not invoke callback");
}
void CheckRegionVisibilityDispatch()
{
    const RECT client{0, 0, 1280, 720};
    URegionInfoPopup direct, polymorphic;
    direct.Setup(GetModuleHandleW(nullptr), client);
    polymorphic.Setup(GetModuleHandleW(nullptr), client);
    UWidget* widget = &polymorphic;
    int directEntered = 0, polymorphicEntered = 0;
    direct.SetOnEnterClick([&]() { ++directEntered; });
    polymorphic.SetOnEnterClick([&]() { ++polymorphicEntered; });
    auto samePresentation = [&]() {
        Require(direct.GetVisible() == polymorphic.GetVisible() &&
            direct.IsPresenting() == polymorphic.IsPresenting() &&
            direct.IsOpening() == polymorphic.IsOpening() &&
            direct.IsClosing() == polymorphic.IsClosing() &&
            direct.IsInputBlocked() == polymorphic.IsInputBlocked() &&
            std::abs(direct.GetPresentationOpacity() - polymorphic.GetPresentationOpacity()) < .000001,
            "Base-pointer visibility differs from concrete popup presentation");
    };
    auto setVisible = [&](bool visible) {
        direct.SetVisible(visible);
        widget->SetVisible(visible);
        samePresentation();
    };
    auto advance = [&](double seconds) {
        direct.AdvancePresentation(seconds);
        polymorphic.AdvancePresentation(seconds);
        samePresentation();
    };
    setVisible(true);
    Require(polymorphic.IsOpening() && polymorphic.GetPresentationOpacity() == 0.0,
        "Base-pointer opening skipped its transparent starting state");
    advance(.1);
    Require(std::abs(polymorphic.GetPresentationOpacity() - .5) < .000001,
        "Base-pointer opening did not reach the expected midpoint");
    advance(.1);

    // Find the real ENTER controls through hit testing, without exposing popup internals.
    auto enterButton = [](URegionInfoPopup& popup) {
        const Location location = popup.GetLocation();
        const BuSIZE size = popup.GetSize();
        auto* button = dynamic_cast<UButton*>(popup.FindWidgetUnderMouse(
            location.x + size.sX / 2 - 112, location.y + size.sY / 2 - 54));
        Require(button != nullptr, "Fully open popup ENTER control is not a hit target");
        return button;
    };
    UButton* directEnter = enterButton(direct);
    UButton* polymorphicEnter = enterButton(polymorphic);
    directEnter->SetIsPressed(true);
    polymorphicEnter->SetIsPressed(true);
    setVisible(false);
    // Do not advance or Update before this assertion: cancellation must be immediate.
    Require(!directEnter->GetIsPressed() && !polymorphicEnter->GetIsPressed(),
        "Base-pointer hide left a button pressed until the next update");
    Require(polymorphic.IsClosing() && polymorphic.IsInputBlocked(),
        "Base-pointer hide failed to retain the closing modal guard");
    advance(.04);
    const double closingOpacity = polymorphic.GetPresentationOpacity();
    Require(closingOpacity > 0.0 && closingOpacity < 1.0,
        "Base-pointer close did not produce an intermediate frame");
    setVisible(true);
    Require(std::abs(polymorphic.GetPresentationOpacity() - closingOpacity) < .000001,
        "Base-pointer reopening jumped away from the displayed opacity");
    advance(.20);
    Click(direct, directEnter);
    Click(polymorphic, polymorphicEnter);
    directEnter->Click(); polymorphicEnter->Click();
    Require(directEntered == 1 && polymorphicEntered == 1 && polymorphic.IsInputBlocked(),
        "Base-pointer popup entry did not preserve the single-commit guard");

    // Hide and reopen in one input turn, before Update can reconcile any state.
    setVisible(false);
    setVisible(true);
    Require(!polymorphic.IsInputBlocked() && !polymorphic.IsOpening(),
        "Base-pointer reopen retained the previous session's committed entry");
    Click(direct, directEnter);
    Click(polymorphic, polymorphicEnter);
    Require(directEntered == 2 && polymorphicEntered == 2,
        "Base-pointer reopened popup could not enter again");
    setVisible(false);
    advance(.16);
    Require(!polymorphic.GetVisible() && !polymorphic.IsPresenting(),
        "Base-pointer close did not finish hiding the popup");
}
std::vector<std::uint32_t> RenderRegion(Canvas& canvas, HBITMAP map, URegionInfoPopup& popup)
{
    canvas.Background(map);
    popup.Draw(canvas.dc, canvas.scratch);
    GdiFlush();
    std::vector<std::uint32_t> pixels(static_cast<size_t>(canvas.width) * canvas.height);
    for (size_t i = 0; i < pixels.size(); ++i) pixels[i] = canvas.pixels[i] & 0x00ffffffu;
    return pixels;
}
std::uint64_t PixelDifference(const std::vector<std::uint32_t>& a, const std::vector<std::uint32_t>& b)
{
    Require(a.size() == b.size(), "Animation comparison dimensions changed");
    std::uint64_t difference = 0;
    for (size_t i = 0; i < a.size(); ++i)
        for (int shift : {0,8,16}) difference += static_cast<unsigned>(std::abs(
            static_cast<int>((a[i] >> shift) & 255) - static_cast<int>((b[i] >> shift) & 255)));
    return difference;
}
void CheckRegionPresentation(RECT client, HBITMAP map, const std::filesystem::path& output)
{
    Canvas canvas(client.right, client.bottom);
    URegionInfoPopup region; region.Setup(GetModuleHandleW(nullptr), client);
    region.SetMapPreview(map, {1140,410,1460,640}, L"Stage 01");
    int entered = 0, closed = 0;
    region.SetOnEnterClick([&]() { ++entered; }); region.SetOnCloseClick([&]() { ++closed; });
    const auto background = RenderRegion(canvas, map, region);
    region.SetInfo(L"묘지", L"현재 기준이 되는 묘지 전투 스테이지입니다.", L"Graveyard");
    Require(region.GetVisible() && region.IsPresenting() && !region.IsClosing(), "Opening popup logical state differs");
    Require(region.GetPresentationOpacity() == 0.0, "Popup opening skipped initial transparent frame");
    Require(region.IsOpening(), "Newly opened popup must remain in opening phase");
    const auto openingEnter = region.btnEnter->GetLocation();
    Require(region.FindWidgetUnderMouse(openingEnter.x, openingEnter.y) != region.btnEnter, "Invisible opening frame exposed ENTER input");
    region.btnEnter->Click(); region.btnClose->Click();
    Require(entered == 0 && closed == 0 && region.GetVisible(), "Opening buttons committed or closed before being visible");
    Require(PixelDifference(background, RenderRegion(canvas, map, region)) == 0, "Initial popup frame altered the background");
    Snapshot(canvas, map, region, output, "region-opening-start");
    region.AdvancePresentation(.1);
    const double middleOpacity = region.GetPresentationOpacity();
    Require(middleOpacity > 0.0 && middleOpacity < 1.0, "Opening mid-frame is not intermediate");
    Require(region.IsOpening() && region.FindWidgetUnderMouse(openingEnter.x, openingEnter.y) != region.btnEnter, "Half-open popup exposed ENTER input");
    region.btnEnter->Click(); region.btnClose->Click();
    Require(entered == 0 && closed == 0, "Half-open popup ran a button callback");
    const auto middle = RenderRegion(canvas, map, region);
    Snapshot(canvas, map, region, output, "region-opening-middle");
    region.AdvancePresentation(.1);
    Require(std::abs(region.GetPresentationOpacity() - 1.0) < .0001, "Popup did not finish opening at .20 seconds");
    Require(!region.IsOpening(), "Fully visible popup retained opening input lock");
    const auto fullyOpen = RenderRegion(canvas, map, region);
    Require(PixelDifference(background, middle) > 0 && PixelDifference(background, fullyOpen) > PixelDifference(background, middle), "Actual GDI opening does not reveal the popup progressively");
    Snapshot(canvas, map, region, output, "region-opening-complete");

    Click(region, region.btnClose);
    Require(closed == 1 && !region.GetVisible() && region.IsPresenting() && region.IsClosing() && region.IsInputBlocked(), "Close must start a visible, input-blocking presentation");
    region.btnClose->Click(); region.btnEnter->Click();
    Require(closed == 1 && entered == 0, "Repeated buttons fired during popup close");
    const auto enter = region.btnEnter->GetLocation();
    Require(region.FindWidgetUnderMouse(enter.x, enter.y) != region.btnEnter, "Closing popup exposes a live ENTER button");
    region.AdvancePresentation(.08);
    Require(region.GetPresentationOpacity() > 0 && region.GetPresentationOpacity() < 1, "Closing mid-frame opacity is not intermediate");
    const auto closing = RenderRegion(canvas, map, region);
    Require(PixelDifference(background, closing) > 0 && PixelDifference(background, closing) < PixelDifference(background, fullyOpen), "Actual GDI closing does not fade progressively");
    Snapshot(canvas, map, region, output, "region-closing-middle");
    region.AdvancePresentation(.08);
    Require(!region.IsPresenting() && !region.IsClosing() && !region.GetVisible(), "Close completion left a visible modal");
    Require(PixelDifference(background, RenderRegion(canvas, map, region)) == 0, "Completed close left pixels behind");

    // Rapid open/close reversal must begin at the current opacity, not jump to an endpoint.
    region.SetVisible(true); region.AdvancePresentation(.07);
    const double opening = region.GetPresentationOpacity();
    region.SetVisible(false);
    Require(std::abs(region.GetPresentationOpacity() - opening) < .0001, "Opening reversal jumped opacity");
    region.AdvancePresentation(.04);
    const double reversed = region.GetPresentationOpacity();
    Require(reversed < opening && reversed > 0, "Reversed opening did not continue toward zero");
    region.SetVisible(true);
    Require(std::abs(region.GetPresentationOpacity() - reversed) < .0001, "Closing reversal jumped opacity");
    region.AdvancePresentation(.20);
    Require(std::abs(region.GetPresentationOpacity() - 1.0) < .0001, "Reopened popup did not finish");
    Click(region, region.btnEnter); region.btnEnter->Click(); region.btnClose->Click();
    Require(entered == 1 && closed == 1 && region.IsInputBlocked(), "Committed entry allowed duplicate/competing callbacks");
    Require(region.rName == L"묘지" && region.rDesc == L"현재 기준이 되는 묘지 전투 스테이지입니다." && region.rEnv == L"Graveyard", "Presentation transitions changed existing region information");
}
void CheckPopupFlow(RECT client, HBITMAP map, const std::filesystem::path& output)
{
    auto& game = GameManager::Get();
    game.Clear(); game.InitDefaultParty();
    Require(game.GetParty().size() == 5 && game.GetGold() == 0 && game.GetInventory().empty(), "Actual fresh-party defaults differ");
    const auto& party = game.GetParty();
    UMainMenuPopup menu;
    menu.Setup(GetModuleHandleW(nullptr), client);
    menu.SetTeamData(party); menu.ShowNavigation();
    Require(menu.currentTab == EMenuTab::CharacterInfo && menu.charSelectButtons.size() == 5, "Main-menu default/status party missing");
    Require(menu.guideLabel->Text == L"캐릭터를 선택하면 능력치와 장비 정보를 확인할 수 있습니다.", "Original main character guide changed");
    Require(menu.invGuideLabel->Text == L"장비 또는 소비 아이템을 선택한 뒤 대상 캐릭터를 고르세요.", "Original main inventory guide changed");
    Require(Button(menu.navLayer, L"상태") && Button(menu.navLayer, L"인벤토리") && Button(menu.navLayer, L"종료") && Button(menu.navLayer, L"X") && Button(menu.navLayer, L"상세 보기") && Button(menu.navLayer, L"가방 보기"), "One of six original navigation controls missing");
    for (size_t i = 0; i < party.size(); ++i) {
        Click(menu, menu.charSelectButtons[i]);
        Require(menu.selectedCharacter == party[i] && menu.profilePanel->currentCharacter == party[i] && menu.statePanel->currentCharacter == party[i], "Member selection did not bind both live panels");
        Require(party[i]->GetLevel() == 1 && party[i]->GetExp() == 0, "Default level/EXP altered");
        Require(party[i]->GetIllustration() != nullptr, "Approved portrait did not load");
    }
    Click(menu, menu.charSelectButtons[0]);
    Canvas canvas(client.right, client.bottom);
    Snapshot(canvas, map, menu, output, "menu-character");
    Click(menu, menu.btnOpenInv);
    Require(menu.currentTab == EMenuTab::Inventory && menu.invEmptyLabel->GetVisible() && menu.itemIconButtons.empty(), "Empty bag not rendered from real inventory");
    Require(menu.invTitleLabel->Text == L"공용 인벤토리    골드: 0", "Gold missing in main inventory");
    Click(menu, menu.btnOpenCharDetail);
    Require(menu.charPopup->GetVisible(), "Character detail did not open from inventory tab");
    Click(*menu.charPopup, menu.charPopup->backButton);
    Require(menu.currentTab == EMenuTab::Inventory && !menu.charPopup->GetVisible(), "Character Back lost original inventory tab");
    Click(menu, menu.btnOpenChar);
    Click(menu, menu.btnOpenInvDetail); menu.Update();
    Click(*menu.invPopup, menu.invPopup->backButton);
    Require(menu.currentTab == EMenuTab::CharacterInfo && !menu.invPopup->GetVisible(), "Inventory Back lost original status tab");
    Click(menu, menu.btnOpenCharDetail);
    for (size_t i = 0; i < party.size(); ++i) {
        Click(*menu.charPopup, menu.charPopup->charSelectButtons[i]);
        Require(menu.charPopup->selectedCharacter == party[i] && menu.charPopup->profilePanel->currentCharacter == party[i] && menu.charPopup->statePanel->currentCharacter == party[i], "Detailed member selection does not share live field source");
    }
    menu.charPopup->SelectCharacter(party[0]);
    Snapshot(canvas, map, menu, output, "character");
    Require(menu.HandleBack() && menu.currentTab == EMenuTab::CharacterInfo, "Character Escape did not return to original tab");

    SeedCatalog();
    game.SetGold(275);
    menu.SelectTab(EMenuTab::Inventory);
    Require(menu.itemIconButtons.size() == ItemData::GetAllItems().size() && menu.itemIconButtons.size() == 10, "All actual item types must be reachable");
    Require(!menu.invEmptyLabel->GetVisible() && menu.invTitleLabel->Text == L"공용 인벤토리    골드: 275", "Live gold/empty state not refreshed");
    for (const auto& item : ItemData::GetAllItems()) {
        auto button = ItemButton(menu.itemIconButtons, item.ItemID);
        Require(button->ButtonText == item.Name + L" x3", "Original item name/count missing");
        Hover(menu, button);
        Require(menu.inspectedItemID == item.ItemID, "Persistent item detail selection failed");
    }
    // All formatted bonus/description branches are independently source-audited;
    // this executable selects the actual items and captures their real Draw output.
    Require(ItemData::GetBonus(302).SPD == -1, "Negative item fixture unavailable");
    Require(ItemData::GetBonus(201).MaxHP == 0 && ItemData::GetBonus(201).MaxMP == 0, "Zero-bonus item fixture unavailable");
    Require(ItemData::GetHealMP(101) == 0, "Zero-healing item fixture unavailable");
    Hover(menu, ItemButton(menu.itemIconButtons, 302));
    Snapshot(canvas, map, menu, output, "menu-inventory");
    Click(menu, ItemButton(menu.itemIconButtons, 201));
    Require(menu.targetSelectPanel->GetVisible(), "Main item did not open target selection");
    Require(menu.FindWidgetUnderMouse(menu.btnExit->GetLocation().x, menu.btnExit->GetLocation().y) != menu.btnExit, "Main target panel allowed navigation click-through");
    const int swordBefore = game.GetItemCount(201), strBefore = party[0]->GetStat().STR;
    Click(menu, Button(menu.targetSelectPanel, L"취소"));
    Require(game.GetItemCount(201) == swordBefore && party[0]->GetStat().STR == strBefore, "Cancel consumed/equipped an item");
    Click(menu, ItemButton(menu.itemIconButtons, 201));
    Snapshot(canvas, map, menu, output, "menu-targets");
    Click(menu, Button(menu.targetSelectPanel, party[0]->GetName()));
    Require(game.GetItemCount(201) == swordBefore - 1 && party[0]->GetEquipment().WeaponID == 201 && party[0]->GetStat().STR == strBefore + 5, "Main target callback did not equip the actual item");
    Require(menu.statePanel->currentCharacter == party[0], "Live state binding was lost after equip");
    CharacterStat injured = party[0]->GetStat(); injured.CurHP = 20; party[0]->SetStat(injured);
    const int potionBefore = game.GetItemCount(101);
    Click(menu, ItemButton(menu.itemIconButtons, 101));
    Click(menu, Button(menu.targetSelectPanel, party[0]->GetName()));
    Require(party[0]->GetStat().CurHP == 70 && game.GetItemCount(101) == potionBefore - 1, "Main target callback did not use/heal with actual potion");

    Click(menu, menu.btnOpenInvDetail); menu.Update();
    Require(menu.invPopup->itemIconButtons.size() == 10, "Detailed bag did not refresh actual items");
    for (const auto& item : ItemData::GetAllItems()) {
        Hover(*menu.invPopup, ItemButton(menu.invPopup->itemIconButtons, item.ItemID));
        Require(menu.invPopup->detailItemID == item.ItemID && menu.invPopup->tooltipTitle == item.Name && menu.invPopup->tooltipDesc == item.Description, "Detailed item hover did not retain full name/description");
    }
    Hover(*menu.invPopup, ItemButton(menu.invPopup->itemIconButtons, 302));
    Require(menu.invPopup->tooltipDesc == ItemData::GetDescription(302), "Detail item description differs from source");
    Snapshot(canvas, map, menu, output, "inventory");
    Click(*menu.invPopup, ItemButton(menu.invPopup->itemIconButtons, 102));
    Require(menu.invPopup->targetButtons.size() == 6, "Detailed item target members/cancel missing");
    Require(menu.invPopup->FindWidgetUnderMouse(menu.invPopup->itemIconButtons[0]->GetLocation().x, menu.invPopup->itemIconButtons[0]->GetLocation().y) != menu.invPopup->itemIconButtons[0], "Detailed target selection allowed item click-through");
    Snapshot(canvas, map, menu, output, "targets");
    Require(menu.HandleBack() && menu.invPopup->GetVisible() && menu.invPopup->selectedItemID < 0, "Escape did not dismiss target before inventory");
    Click(*menu.invPopup, ItemButton(menu.invPopup->itemIconButtons, 302));
    const int armorBefore = game.GetItemCount(302), spdBefore = party[0]->GetStat().SPD;
    Click(*menu.invPopup, menu.invPopup->targetButtons[0]); menu.Update();
    Require(game.GetItemCount(302) == armorBefore - 1 && party[0]->GetEquipment().ArmorID == 302 && party[0]->GetStat().SPD == spdBefore - 1, "Detailed target callback lost armor negative bonus");
    CharacterStat drained = party[0]->GetStat(); drained.CurMP = 0; party[0]->SetStat(drained);
    const int manaBefore = game.GetItemCount(102);
    Click(*menu.invPopup, ItemButton(menu.invPopup->itemIconButtons, 102));
    Click(*menu.invPopup, menu.invPopup->targetButtons[0]); menu.Update();
    Require(party[0]->GetStat().CurMP == (std::min)(30, party[0]->GetStat().MaxMP) && game.GetItemCount(102) == manaBefore - 1, "Detailed target callback did not use actual mana potion");
    Require(menu.HandleBack() && !menu.invPopup->GetVisible() && menu.currentTab == EMenuTab::Inventory, "Second Escape did not return from inventory to original tab");
    Require(!menu.HandleBack(), "Plain menu must allow stage-level Escape to close it");
    Click(menu, menu.btnExit);
    Require(menu.FindWidgetUnderMouse(menu.btnOpenInv->GetLocation().x, menu.btnOpenInv->GetLocation().y) != menu.btnOpenInv, "Exit confirmation allowed menu click-through");
    Snapshot(canvas, map, menu, output, "exit");
    Click(menu, menu.exitPopup->btnNo);
    Require(!menu.exitPopup->GetVisible(), "Exit no did not dismiss confirmation");
    Click(menu, menu.btnExit);
    MSG message{};
    PeekMessageW(&message, nullptr, 0, 0, PM_NOREMOVE); // creates only this thread's queue
    Click(menu, menu.exitPopup->btnYes);
    Require(PeekMessageW(&message, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE) && message.message == WM_QUIT, "Real exit callback did not post WM_QUIT");
    Require(!PeekMessageW(&message, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE), "WM_QUIT was not fully drained");
    menu.exitPopup->SetVisible(false);

    USettingPopup settings; settings.Setup(GetModuleHandleW(nullptr), client); settings.SetVisible(true);
    Require(settings.bSoundOn && settings.btnSoundToggle->ButtonText == L"사운드: 켜짐", "Initial sound label differs");
    Click(settings, settings.btnSoundToggle);
    Require(!settings.bSoundOn && settings.btnSoundToggle->ButtonText == L"사운드: 꺼짐", "Local sound label toggle failed");
    Snapshot(canvas, map, settings, output, "settings");
    Click(settings, settings.btnClose); Require(!settings.GetVisible(), "Settings CLOSE failed");
    UNoticePopup notice; notice.Setup(client); notice.ShowNotice(L"잠김", L"이전 스테이지를 먼저 클리어하세요.");
    Require(notice.TitleText == L"잠김" && notice.MessageText == L"이전 스테이지를 먼저 클리어하세요.", "Notice original title/message lost");
    Snapshot(canvas, map, notice, output, "notice");
    Click(notice, notice.btnOK); Require(!notice.IsOpened(), "Notice OK failed");
    URegionInfoPopup region; region.Setup(GetModuleHandleW(nullptr), client);
    region.SetMapPreview(map, {1140,410,1460,640}, L"Stage 01");
    region.SetInfo(L"묘지", L"현재 기준이 되는 묘지 전투 스테이지입니다.", L"Graveyard");
    int entered = 0, closed = 0;
    region.SetOnEnterClick([&]() { ++entered; }); region.SetOnCloseClick([&]() { ++closed; });
    Require(region.rName == L"묘지" && region.rEnv == L"Graveyard" && region.parentRegion == L"Stage 01", "Region original fields lost");
    region.AdvancePresentation(.20);
    Snapshot(canvas, map, region, output, "region");
    Click(region, region.btnEnter); Require(entered == 1, "Region ENTER callback not relayed");
    // ENTER commits a route once. CLOSE is a separate popup session, not a competing action.
    region.SetVisible(false); region.AdvancePresentation(.16);
    region.SetInfo(L"묘지", L"현재 기준이 되는 묘지 전투 스테이지입니다.", L"Graveyard");
    region.AdvancePresentation(.20);
    Click(region, region.btnClose); Require(closed == 1 && !region.GetVisible(), "Region CLOSE callback/state not relayed");
}

LPARAM At(UButton* button) { const auto p = button->GetLocation(); return MAKELPARAM(p.x, p.y); }
void CheckStageInput()
{
    const RECT client{0,0,1280,720};
    Map_Stage map;
    map.Camera = new ACamera(0,0,0,0);
    map.rects = client;
    map.infoPopup = new URegionInfoPopup(); map.infoPopup->Setup(GetModuleHandleW(nullptr), client);
    map.mainMenu = new UMainMenuPopup(); map.mainMenu->Setup(GetModuleHandleW(nullptr), client);
    map.noticePopup = new UNoticePopup(); map.noticePopup->Setup(client);
    map.screenUI.AddChildWindow(map.infoPopup); map.screenUI.AddChildWindow(map.mainMenu);
    map.Input(nullptr, WM_KEYDOWN, 'I', 0); map.mainMenu->Update();
    Require(map.mainMenu->invPopup->GetVisible(), "Map I shortcut did not open actual inventory");
    map.Input(nullptr, WM_KEYDOWN, 'P', 0);
    Require(map.mainMenu->invPopup->GetVisible() && !map.mainMenu->charPopup->GetVisible(), "Modal I/P shortcut stacked another popup");
    Click(*map.mainMenu->invPopup, ItemButton(map.mainMenu->invPopup->itemIconButtons, 101));
    map.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0);
    Require(map.mainMenu->invPopup->GetVisible() && !map.mainMenu->invPopup->targetPanel->GetVisible(), "Map Escape did not dismiss target first");
    map.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0);
    Require(map.mainMenu->GetVisible() && !map.mainMenu->invPopup->GetVisible(), "Map Escape did not return to menu");
    map.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0);
    Require(!map.mainMenu->GetVisible(), "Map Escape did not close plain menu");
    map.Input(nullptr, WM_KEYDOWN, 'P', 0);
    Require(map.mainMenu->charPopup->GetVisible(), "Map P shortcut did not open character detail");
    map.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0);
    Require(map.mainMenu->currentTab == EMenuTab::CharacterInfo, "Map character back changed selected tab");
    auto* status = map.mainMenu->btnOpenChar; auto* inventory = map.mainMenu->btnOpenInv;
    map.Input(nullptr, WM_LBUTTONDOWN, 0, At(inventory));
    map.Input(nullptr, WM_LBUTTONUP, 0, At(status));
    Require(map.mainMenu->currentTab == EMenuTab::CharacterInfo && !inventory->GetIsPressed(), "Release over a different button activated/retained old press");
    map.Input(nullptr, WM_LBUTTONUP, 0, At(inventory));
    Require(map.mainMenu->currentTab == EMenuTab::CharacterInfo, "Release without press activated menu");
    map.Input(nullptr, WM_LBUTTONDOWN, 0, At(inventory)); map.Input(nullptr, WM_LBUTTONUP, 0, At(inventory));
    Require(map.mainMenu->currentTab == EMenuTab::Inventory, "Matching down/up did not activate actual navigation");
    map.noticePopup->ShowNotice(L"획득", L"Small Potion을(를) 획득했습니다.");
    map.Input(nullptr, WM_KEYDOWN, 'P', 0);
    Require(map.noticePopup->IsOpened() && !map.mainMenu->charPopup->GetVisible(), "Notice modal leaked shortcut input");
    map.Input(nullptr, WM_KEYDOWN, VK_RETURN, 0); Require(!map.noticePopup->IsOpened(), "Notice Enter did not close");
    map.mainMenu->SetVisible(false);
    map.infoPopup->SetInfo(L"기본 방", L"기믹 없는 기본 전투 확인용 스테이지입니다.", L"Default");
    map.infoPopup->AdvancePresentation(.1);
    Require(map.infoPopup->IsOpening(), "Stage fixture did not reach half-open popup");
    map.Input(nullptr, WM_KEYDOWN, 'I', 0); Require(!map.mainMenu->GetVisible(), "Region modal leaked inventory shortcut");
    map.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0); Require(!map.infoPopup->GetVisible(), "Region Escape did not close");
    Require(map.infoPopup->IsClosing(), "Escape during opening did not reverse the popup smoothly");
    map.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0);
    Require(!map.mainMenu->GetVisible(), "Repeated Escape during close opened the map menu");
    map.infoPopup->AdvancePresentation(.16);
    Require(!map.infoPopup->IsPresenting(), "Reversed popup did not finish closing");
    Title_Stage title;
    title.settingPopup = new USettingPopup(); title.settingPopup->Setup(GetModuleHandleW(nullptr), client);
    title.mainHUD.AddChildWindow(title.settingPopup); title.settingPopup->SetVisible(true);
    title.Input(nullptr, WM_KEYDOWN, VK_ESCAPE, 0);
    Require(!title.settingPopup->GetVisible() && title.NextStageID == -2, "Title settings Escape attempted to leave/quit title");
}
}

int wmain(int argc, wchar_t** argv)
{
    try {
        Require(argc == 3, "Usage: popup_ui_interaction_checks.exe <game-source-directory> <output-directory>");
        const auto source = std::filesystem::absolute(argv[1]);
        const auto output = std::filesystem::absolute(argv[2]);
        std::filesystem::create_directories(output);
        std::filesystem::current_path(source);
        HBITMAP map = static_cast<HBITMAP>(LoadImageW(nullptr, L"Source/SMap/background/Map_Background.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION));
        Require(map != nullptr, "Approved world map failed to load");
        CheckButtonGuards();
        CheckRegionVisibilityDispatch();
        CheckPopupFlow({0,0,1600,900}, map, output);
        CheckPopupFlow({0,0,1280,720}, map, output);
        CheckRegionPresentation({0,0,1600,900}, map, output);
        CheckRegionPresentation({0,0,1280,720}, map, output);
        CheckStageInput();
        DeleteObject(map);
        std::ofstream report(output / "interaction-checks.json");
        report << "{\n  \"passed\": true,\n  \"assertions\": " << checks << ",\n  \"snapshotWidths\": [1600,1280],\n  \"scope\": \"Production callbacks, stage Input, in-memory GameManager and real GDI popup Draw. No stage Init, window, gameplay loop, audio or disk save. WM_QUIT verified and drained.\"\n}\n";
        std::cout << "PASS: " << checks << " actual popup/input assertions; GDI snapshots written.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
