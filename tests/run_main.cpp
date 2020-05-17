#include "ApprovalTests.hpp"
#include "catch2/catch.hpp"

#include "mock_main.hpp"

using namespace ApprovalTests;


TEST_CASE("Launch_Doom_Main")
{
    auto demo = GENERATE("02NOVIEW.LMP", "E1M2SAW.LMP", "E2M4ALL.LMP"/*, "E3M9STUK.LMP", "FUSEDEM2.LMP",
                         "07INTOWR.LMP", "E1M2SEC.LMP", "E2M4EXIT.LMP", "E4M1ALL.LMP", "GARR-XPE.LMP",
                         "07TOWER.LMP", "E1M3SEC.LMP", "E2M4STUK.LMP", "E4M1SWCH.LMP", "GHOSTPE1.LMP",
                         "22NOVIEW.LMP", "E1M4EXIT.LMP", "E2M4TRAP.LMP", "E4M1WIN.LMP", "GHOSTPE2.LMP",
                         "2GHOSTS.LMP", "E1M4TRIK.LMP", "E2M6RED.LMP", "E4M2BFGX.LMP", "IMPFIGHT.LMP",
                         "BFGDEMO.LMP", "E1M4TRK2.LMP", "E2M8WIN1.LMP", "E4M2KEYS.LMP", "IMPGHOST.LMP",
                         "BLINDAV.LMP", "E1M5SEC.LMP", "E2M8WIN2.LMP", "E4M2ROOM.LMP", "IMPMISS2.LMP",
                         "BOHFIGHT.LMP", "E1M6ALT.LMP", "E2M9WIN.LMP", "E4M2SWCH.LMP", "IMPMISS.LMP",
                         "BOUNCE1.LMP", "E1M6KEYS.LMP", "E3M2TRAP.LMP", "E4M2TRIK.LMP", "MANORMIS.LMP",
                         "BOUNCE2.LMP", "E1M7ALL.LMP", "E3M4SWCH.LMP", "E4M3SEC.LMP", "NONTOXIC.LMP",
                         "BURNTHRU.LMP", "E1M7SAW.LMP", "E3M5BOXS.LMP", "E4M3STUK.LMP", "REVFIGHT.LMP",
                         "CACOFIT2.LMP", "E1M7SOUL.LMP", "E3M5KEYS.LMP", "E4M3SWCH.LMP", "REVORBIT.LMP",
                         "CACOFITE.LMP", "E1M8BACK.LMP", "E3M5YSEC.LMP", "E4M4OTHR.LMP", "SMPUNCH.LMP",
                         "COMBSLIP.LMP", "E1M8WIN1.LMP", "E3M6BLUE.LMP", "E4M4SEC.LMP", "SOULFRAG.LMP",
                         "D1SHOCK.LMP", "E1M8WIN2.LMP", "E3M6BOX.LMP", "E4M4SK12.LMP", "SOULPOP.LMP",
                         "D1SLIDE.LMP", "E1M9ALL.LMP", "E3M6EXIT.LMP", "E4M6HOLE.LMP", "SPEEDBMP.LMP",
                         "D2SHOCK.LMP", "E1M9JUMP.LMP", "E3M6PUFF.LMP", "E4M6KEYS.LMP", "STILHERE.LMP",
                         "D2SLIDE.LMP", "E1M9PEDS.LMP", "E3M6SAW.LMP", "E4M6UXO.LMP", "STUCKIMP.LMP",
                         "DEADLIFT.LMP", "E2M1ALL.LMP", "E3M7ALL.LMP", "E4M7EXIT.LMP", "TALLBOMB.LMP",
                         "DEDNGONE.LMP", "E2M2TRIK.LMP", "E3M7ALT.LMP", "FIREWALL.LMP", "TAPFOCUS.LMP",
                         "E1M1SEC.LMP", "E2M3SEC.LMP", "E3M8WIN.LMP", "FLICKER.LMP", "THINGRUN.LMP",
                         "E1M1SWCH.LMP", "E2M3SUIT.LMP", "E3M9EXIT.LMP", "FUSEDEM1.LMP", "WALLRUN.LMP"*/
            );
    SECTION("Run Main")
    {
        RunDoomMain(demo);
    }
}