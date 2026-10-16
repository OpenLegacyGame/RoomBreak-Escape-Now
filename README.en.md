[简体中文](README.md) · [繁體中文](README.zh-Hant.md) · [日本語](README.ja.md) · [English](README.en.md) · [한국어](README.ko.md)

# RoomBreak Escape Now

![RoomBreak Escape Now title screen](media/title.png)

## Open that door again

RoomBreak: Escape Now!! is GAMEDAY's story-driven escape game, released on iOS in the US on [September 23, 2011](https://gamefaqs.gamespot.com/iphone/644223-roombreak-escape-now/data). Across six stories and thirty levels, you search for clues, combine items, solve mechanisms, and use tools such as hammers and knives to find a way forward.

The original relies on early mobile devices and aging software environments. Replaying it on today's phones and computers has become difficult. This project aims to bring its familiar rooms, sounds and stories to today's phones and computers as **native applications for macOS, Windows, Android and iOS**.

The Korean business-information website Bizno lists GAMEDAY Inc. (registration 617-81-59967) as having [ceased business on February 17, 2021](https://bizno.net/article/6178159967). This is the business status recorded by that website; rights to the original content remain with the respective rights holders.

## Playing today

- Six episodes and thirty main stages, with a tutorial, story scenes, item combinations and puzzle minigames.
- In-game Chinese, English, Japanese and Korean, with mouse and touch controls.
- Free offline shop claims, keeping the original quantities per product and allowing repeated claims.
- Original starting item quantities on a fresh save; later gains and consumption persist across restarts.
- An “Unlock all stages” shop button with confirmation.
- Gold keys remain one-time collectibles, with collection recorded in your save.

## Walkthrough

Stuck on a room? The [complete walkthrough](walkthrough/en.md) covers all thirty stages chapter by chapter, with control tips.

## Join the discussion

Come talk about the game and the project on Telegram:

- Channel: [@OpenLegacyGame](https://t.me/OpenLegacyGame)
- Group: [@OpenLegacyGameGroup](https://t.me/OpenLegacyGameGroup)

## Familiar play, a modern implementation

The original iOS game used **Objective-C and cocos2d**, targeting 32-bit iOS and OpenGL ES. This version rewrites the runtime in **C++ with Axmol 2.11.5**, sharing scenes, story logic, items, puzzles, audio and saves across four platforms while using modern graphics backends.

We also fixed an original safe-dial issue: the old implementation truncated drag movement smaller than one unit to zero, so a slow, fine adjustment could go unnoticed. The new implementation retains floating-point precision, allowing small movements to turn the dial. Interface text preserves the original font style with improved clarity at higher window resolutions.

## Gameplay clips

### Hammer minigame

[Watch the hammer clip](media/hammer.mp4)

### Cutting minigame

[Watch the cutting clip](media/knife.mp4)

> The original RoomBreak: Escape Now!! name, artwork, music, story and level content belong to GAMEDAY Inc. and their respective rights holders. This is an independent modern reimplementation, unaffiliated with the original developer. The MIT license covers newly written program code and does not relicense original game content.

## About this recreation

I'm **Astra 6**, the AI development assistant involved in rewriting and validating this project. From studying the original behavior to recreating it for modern platforms, this work took approximately **8 hours**, with an estimated **40% of a 20× weekly allowance** used for this task. These are approximate figures for this particular effort, not a fixed cost for similar projects.

May the stories once kept on old phones continue on today's screens.

## Code refactoring

I'm **Claude Opus 5.5 (High)**, responsible for the subsequent refactoring and optimization, with the aim of preserving existing gameplay behavior.

The refactoring focuses on readability and maintainability: a consistent code style, clear module responsibilities and less duplicated code. Game data lookups use indexes that keep the original data order and avoid repeated scanning.

**Original content © GAMEDAY Inc. and respective rights holders. Newly written program code is MIT-licensed.**
