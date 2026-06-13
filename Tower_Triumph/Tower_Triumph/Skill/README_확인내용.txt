Tower Triumph Skill Effect BMP 정리본

[내부 확인 결과]
- 기준 파일: Tower_Triumph_stage_maps_patch_v19.zip 내부 tt_patch/SkillData.cpp
- 코드상 스킬 수: 45개
- 구조: Warrior 15개 / Archer 15개 / Mage 15개
- 각 직업별 속성 구조: Fire, Water, Grass, Light, Dark 각 3개씩
- ID 규칙: 1xxx Warrior, 2xxx Archer, 3xxx Mage / 두 번째 자리 1=Fire, 2=Water, 3=Grass, 4=Light, 5=Dark

[업로드 ZIP 확인]
- 업로드된 ZIP 항목 수: 46개
- 완전 동일 이미지 중복: 3개
- 고유 이미지 수: 43개
- 누락된 2개는 현재 세션에 남아 있던 생성본으로 보충: 마법_에너지_슬래시_효과.png, 마법_소환_효과_단계.png
- 최종 BMP 출력 수: 45개

[주의]
- 현재 코드는 아직 스킬 ID와 이펙트 파일명을 자동 연결하지 않습니다.
- 이 ZIP은 SkillData.cpp의 스킬 목록 기준으로 파일명/폴더를 맞춰 정리한 리소스 묶음입니다.
- 세부 매칭은 skill_effect_mapping.csv에서 확인할 수 있습니다.

[폴더 구조]
- Warrior/
  - Fire/ (3개)
    - Warrior/Fire/1101_Flame_Slash.bmp | 1101 Flame Slash | Single | Range 1 | MP 4
    - Warrior/Fire/1102_Burning_Cleave.bmp | 1102 Burning Cleave | Splash1 | Range 1 | MP 7
    - Warrior/Fire/1103_Blaze_Break.bmp | 1103 Blaze Break | Single | Range 1 | MP 6
  - Water/ (3개)
    - Warrior/Water/1201_Aqua_Slash.bmp | 1201 Aqua Slash | Single | Range 1 | MP 4
    - Warrior/Water/1202_Wave_Cutter.bmp | 1202 Wave Cutter | Line | Range 3 | MP 7
    - Warrior/Water/1203_Tide_Breaker.bmp | 1203 Tide Breaker | Single | Range 1 | MP 6
  - Grass/ (3개)
    - Warrior/Grass/1301_Vine_Slash.bmp | 1301 Vine Slash | Single | Range 1 | MP 4
    - Warrior/Grass/1302_Thorn_Cleave.bmp | 1302 Thorn Cleave | Splash1 | Range 1 | MP 7
    - Warrior/Grass/1303_Root_Crusher.bmp | 1303 Root Crusher | Single | Range 1 | MP 6
  - Light/ (3개)
    - Warrior/Light/1401_Holy_Slash.bmp | 1401 Holy Slash | Single | Range 1 | MP 4
    - Warrior/Light/1402_Radiant_Break.bmp | 1402 Radiant Break | Single | Range 1 | MP 7
    - Warrior/Light/1403_Judgment_Cleave.bmp | 1403 Judgment Cleave | Splash1 | Range 1 | MP 8
  - Dark/ (3개)
    - Warrior/Dark/1501_Dark_Slash.bmp | 1501 Dark Slash | Single | Range 1 | MP 4
    - Warrior/Dark/1502_Shadow_Break.bmp | 1502 Shadow Break | Single | Range 1 | MP 7
    - Warrior/Dark/1503_Abyss_Cleave.bmp | 1503 Abyss Cleave | Splash1 | Range 1 | MP 8
- Archer/
  - Fire/ (3개)
    - Archer/Fire/2101_Flame_Arrow.bmp | 2101 Flame Arrow | Single | Range 4 | MP 5
    - Archer/Fire/2102_Explosive_Arrow.bmp | 2102 Explosive Arrow | Splash1 | Range 3 | MP 8
    - Archer/Fire/2103_Blazing_Snipe.bmp | 2103 Blazing Snipe | Single | Range 5 | MP 9
  - Water/ (3개)
    - Archer/Water/2201_Aqua_Arrow.bmp | 2201 Aqua Arrow | Single | Range 4 | MP 5
    - Archer/Water/2202_Piercing_Wave.bmp | 2202 Piercing Wave | Line | Range 5 | MP 8
    - Archer/Water/2203_Tidal_Snipe.bmp | 2203 Tidal Snipe | Single | Range 5 | MP 9
  - Grass/ (3개)
    - Archer/Grass/2301_Thorn_Arrow.bmp | 2301 Thorn Arrow | Single | Range 4 | MP 5
    - Archer/Grass/2302_Vine_Volley.bmp | 2302 Vine Volley | Splash1 | Range 3 | MP 8
    - Archer/Grass/2303_Root_Snipe.bmp | 2303 Root Snipe | Single | Range 5 | MP 9
  - Light/ (3개)
    - Archer/Light/2401_Holy_Arrow.bmp | 2401 Holy Arrow | Single | Range 4 | MP 5
    - Archer/Light/2402_Radiant_Shot.bmp | 2402 Radiant Shot | Single | Range 5 | MP 8
    - Archer/Light/2403_Judgment_Rain.bmp | 2403 Judgment Rain | Splash2 | Range 4 | MP 12
  - Dark/ (3개)
    - Archer/Dark/2501_Dark_Arrow.bmp | 2501 Dark Arrow | Single | Range 4 | MP 5
    - Archer/Dark/2502_Shadow_Pierce.bmp | 2502 Shadow Pierce | Line | Range 5 | MP 8
    - Archer/Dark/2503_Abyss_Snipe.bmp | 2503 Abyss Snipe | Single | Range 5 | MP 10
- Mage/
  - Fire/ (3개)
    - Mage/Fire/3101_Fire_Bolt.bmp | 3101 Fire Bolt | Single | Range 3 | MP 6
    - Mage/Fire/3102_Flame_Burst.bmp | 3102 Flame Burst | Splash1 | Range 3 | MP 10
    - Mage/Fire/3103_Meteor_Burst.bmp | 3103 Meteor Burst | Splash2 | Range 4 | MP 16
  - Water/ (3개)
    - Mage/Water/3201_Water_Shot.bmp | 3201 Water Shot | Single | Range 3 | MP 6
    - Mage/Water/3202_Aqua_Wave.bmp | 3202 Aqua Wave | Line | Range 4 | MP 10
    - Mage/Water/3203_Tidal_Blast.bmp | 3203 Tidal Blast | Splash1 | Range 3 | MP 13
  - Grass/ (3개)
    - Mage/Grass/3301_Vine_Whip.bmp | 3301 Vine Whip | Single | Range 3 | MP 6
    - Mage/Grass/3302_Thorn_Field.bmp | 3302 Thorn Field | Splash1 | Range 3 | MP 10
    - Mage/Grass/3303_Root_Storm.bmp | 3303 Root Storm | Splash2 | Range 4 | MP 16
  - Light/ (3개)
    - Mage/Light/3401_Holy_Ray.bmp | 3401 Holy Ray | Single | Range 4 | MP 7
    - Mage/Light/3402_Smite_Undead.bmp | 3402 Smite Undead | Single | Range 4 | MP 11
    - Mage/Light/3403_Sanctuary_Burst.bmp | 3403 Sanctuary Burst | Splash2 | Range 4 | MP 16
  - Dark/ (3개)
    - Mage/Dark/3501_Dark_Ball.bmp | 3501 Dark Ball | Single | Range 4 | MP 7
    - Mage/Dark/3502_Shadow_Spike.bmp | 3502 Shadow Spike | Single | Range 4 | MP 11
    - Mage/Dark/3503_Abyss_Blast.bmp | 3503 Abyss Blast | Splash2 | Range 4 | MP 16