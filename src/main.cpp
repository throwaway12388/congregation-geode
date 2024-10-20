#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/LevelSelectLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>

#include <random>

using namespace geode::prelude;

int clvlid = 9192910; // Custom level ID?

GJGameLevel* orgLevel = nullptr;
std::string orgLevelString; // for the original Congregation level's string without the startpos
bool jumpscare = false;
int type = 0; // 1: main level, 2: editor level, 3: online level
int mainLevels[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,1001,1002,1003,2001,2002,2003,2004,2005,2006,2007,2008,2009,2010,3001,4001,4002,4003,5001,5002,5003,5004};
std::string startPos = "1,31,2,24525,3,1605,155,3,36,1,kA2,0,kA3,0,kA8,0,kA4,1,kA9,1,kA10,0,kA22,0,kA23,0,kA24,0,kA27,1,kA40,1,kA41,1,kA42,1,kA28,0,kA29,0,kA31,1,kA32,1,kA36,0,kA43,0,kA44,0,kA45,1,kA33,1,kA34,1,kA35,0,kA37,1,kA38,1,kA39,1,kA19,0,kA26,0,kA20,0,kA21,0,kA11,0;";


class $modify(PlayLayer) {
	static void onModify(auto& self) {
        self.setHookPriority("PlayLayer::init", INT_MIN);
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		auto chance = Mod::get()->getSettingValue<double>("chance");

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<float> distrib(0.f, 100.f);
		
		if (distrib(gen) < chance) {
			orgLevel = level;
			level = GameLevelManager::get()->getSavedLevel(clvlid);
			if (level->m_levelNotDownloaded) {
				level = orgLevel;
				GameLevelManager::get()->downloadLevel(clvlid, false);
				jumpscare = false;
			} else {
				if (orgLevelString.compare("")) 
					level->m_levelString = orgLevelString;
				
				if (Mod::get()->getSettingValue<bool>("drop")) {
					if (!orgLevelString.compare("")) 
						orgLevelString = level->m_levelString;
					
					std::string levelString = ZipUtils::decompressString(level->m_levelString, true, 0);
					// add a startpos at the drop of the level
					level->m_levelString = ZipUtils::compressString(levelString + startPos, true, 0);
				}
				
				jumpscare = true;

				if (orgLevel->m_levelType == GJLevelType::Local || (std::find(mainLevels, mainLevels + sizeof(mainLevels)/sizeof(mainLevels[0]), orgLevel->m_levelID.value()) != mainLevels + sizeof(mainLevels)/sizeof(mainLevels[0])))
					type = 1;
				else if (orgLevel->m_levelType == GJLevelType::Editor)
					type = 2;
				else 
					type = 3;

				if (orgLevel->m_levelID.value() == clvlid) {
					level->m_levelString = orgLevelString;
					level = orgLevel;
					jumpscare = false;
				}
			}
		}
		
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

		if (Mod::get()->getSettingValue<bool>("hide") && jumpscare) {
			if (Loader::get()->isModLoaded("prevter.openhack")) {
				if (auto a = this->getChildByID("openhack-startpos-label")) a->setVisible(false);
			}

			if (Loader::get()->isModLoaded("TheSillyDoggo.StartposSwitcher")) {
				CCArrayExt<CCNode*> uiChildren = getChildOfType<UILayer>(this, 0)->getChildren();
				for (auto* child : uiChildren) {
					if (child->getChildrenCount() == 3 && child->getZOrder() == 0 && child->getID() == "") {
						child->setVisible(false);
					}
					if (Loader::get()->isModLoaded("absolllute.megahack")) {
						if ((child->getChildrenCount() == 21 || child->getChildrenCount() == 19) && child->getTag() == 4326 && child->getZOrder() == 99) {
							// 21 is (hopefully) percentage, 19 is testmode
							child->setVisible(false);

						}
					}
				}
			}

			if (Loader::get()->isModLoaded("mat.run-info")) {
				CCArrayExt<CCNode*> plChildren = this->getChildren();
				for (auto* child : plChildren) {
					if (child->getChildrenCount() == 3 && child->getZOrder() == 999) {
						child->setVisible(false); // run info widget
					}	
				}
			}

			if (Loader::get()->isModLoaded("absolllute.megahack")) {
				// LOL
				if (auto a = getChildOfType<UILayer>(this, 0)->getChildByID("absolllute.megahack/startpos-switcher-menu")) a->setPositionX(9999);
			}
		}

		return true;
    }

	void setupHasCompleted() {
		PlayLayer::setupHasCompleted();

		if (Mod::get()->getSettingValue<bool>("hide") && jumpscare) {
			CCArrayExt<CCNode*> plChildren = this->getChildren();
			CCNode* mainNode = nullptr;
			for (auto* child : plChildren) {
				if (Loader::get()->isModLoaded("mat.run-info")) {
					if (child->getChildrenCount() == 3 && child->getZOrder() == 999) {
						child->setVisible(false); // run info widget
					}
				}
				if (child->getZOrder() == -1) mainNode = child;
				if (child->getZOrder() == 10 && typeinfo_cast<CCSprite*>(child)) child->setVisible(false); // progress bar
				if (child->getZOrder() == 15 && typeinfo_cast<CCLabelBMFont*>(child)) child->setVisible(false); // percentage label
				if (child->getZOrder() == 1000 && child->getChildrenCount() == 8) child->setVisible(false); // testmode text on mac
				// if (child->getZOrder() == 1000) child->setVisible(false); // testmode text & all crystal clients nodes
			}

			// hide attempt count
			CCArrayExt<CCNode*> mainChildren = mainNode->getChildren();
			for (auto* child : mainChildren) {
				if (typeinfo_cast<CCLayer*>(child)) {
					if (typeinfo_cast<GJGroundLayer*>(child)) continue;
					else {
						if (auto a = getChildOfType<CCLabelBMFont>(child, 0)) a->setVisible(false);
					}
				}
			}
		}
	}
};

class $modify(LevelInfoLayer) {
	static LevelInfoLayer* create(GJGameLevel* level, bool p1) {
		if (jumpscare && type == 3) {
			// for exiting to the original level's LevelInfoLayer
			level = orgLevel;

			jumpscare = false;
			orgLevel = nullptr;
		}
		return LevelInfoLayer::create(level, p1);
	}
};

class $modify(LevelSelectLayer) {
	bool init(int p0) {
		if (jumpscare && type == 1) {
			// for exiting to the original main level's LevelSelectLayer
			p0 = orgLevel->m_levelID.value() - 1;

			jumpscare = false;
			orgLevel = nullptr;
		}
		return LevelSelectLayer::init(p0);
	}
};

class $modify(EditorPauseLayer) {
	// for exiting to the original level's EditLevelLayer from the editor's pause menu
	void onSaveAndExit(CCObject* sender) {
		EditorPauseLayer::onSaveAndExit(sender);
		if (jumpscare) {
			if (type == 2) {
				auto scene = EditLevelLayer::scene(orgLevel);
				CCDirector::get()->replaceScene(scene);

				orgLevel = nullptr;
			}

			jumpscare = false;
		}
	}

	void FLAlert_Clicked(FLAlertLayer* p0, bool p1) {
		EditorPauseLayer::FLAlert_Clicked(p0, p1);
		if (jumpscare) {
			if (type == 2) {
				auto scene = EditLevelLayer::scene(orgLevel);
				CCDirector::get()->replaceScene(scene);

				orgLevel = nullptr;
			}

			jumpscare = false;
		}
	}
};

class $modify(PauseLayer) {
	void onQuit(CCObject* sender) {
		PauseLayer::onQuit(sender);
		if (jumpscare) {
			if (type == 2) {
				// for exiting to the original level's EditLevelLayer from the pause menu
				auto scene = EditLevelLayer::scene(orgLevel);
				CCDirector::get()->replaceScene(scene);

				orgLevel = nullptr;
			}
			jumpscare = false;
		}
	}
};


$on_mod(Loaded) {
	auto GLM = GameLevelManager::get();
	auto MDM = MusicDownloadManager::sharedState();

	GLM->downloadLevel(clvlid, false);
	#ifdef GEODE_IS_ANDROID
		std::filesystem::path p = MDM->pathForSong(895761).c_str();
		if (!std::filesystem::exists(p.parent_path() / "895761.mp3"))
			std::filesystem::copy(Mod::get()->getResourcesDir() / "895761.mp3", p.parent_path() / "895761.mp3");
	#else
		if (!MDM->isSongDownloaded(895761)) 
			std::filesystem::copy(Mod::get()->getResourcesDir() / "895761.mp3", std::filesystem::path(MDM->pathForSong(895761).c_str()));
	#endif

	// specifically for level id 2004!!
	GLM->storeUserName(5807651, 540196, "Presta");
}
