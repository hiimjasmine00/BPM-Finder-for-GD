#include <Geode/Geode.hpp>
#include "MiniBpm.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"

#include <fstream>
#include <vector>

using namespace geode::prelude;

float detectBPM(const std::string& filePath) {
    //Uses mini mp3 to decode the mp3 files, then using mini bpm to find the bpm

    //Read the file
    std::ifstream file(filePath, std::ios::binary);
    std::vector<uint8_t> fileData((std::istreambuf_iterator<char>(file)), {});
    
    //Decode mp3
    mp3dec_t mp3d;
    mp3dec_init(&mp3d);
    mp3dec_frame_info_t info;
    std::vector<float> samples;
    
    size_t offset = 0;
    while (offset < fileData.size()) {
        short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
        int decoded = mp3dec_decode_frame(&mp3d, fileData.data() + offset, fileData.size() - offset, pcm, &info); //Decodes mp3
        if (decoded == 0) break;
        
        for (int i = 0; i < decoded; i++) {
            samples.push_back(pcm[i * info.channels] / 32768.0f); //Got this float value from claude. I know, i know, i used SOME ai.

            if (samples.size() > static_cast<int>(std::round(info.hz * (60 * 5)))) break;
            //generates samples for mini bpm, long songs like Flamewall won't work cus it will be too long :(
        }
        offset += info.frame_bytes;
    }
    
    if (samples.size() == 0) {
        log::error("Invalid sample count: {}", samples.size());
        return 0.0f;
    }

    //Detect bpm
    breakfastquay::MiniBPM detector(info.hz);
    return detector.estimateTempoOfSamples(samples.data(), samples.size());
}

#include <Geode/modify/EditorUI.hpp>
class $modify(MyEditorUILayer, EditorUI){
    static std::string getSongFilePath(){
        //Gets the song file path

        auto editorLayer = LevelEditorLayer::get();
        if (!editorLayer) return "None";
        
        auto level = editorLayer->m_level;
        if (!level) return "None";
        
        if (level->m_songID != 0) {
            int songID = level->m_songID;

            auto musicManager = MusicDownloadManager::sharedState();
            if (!musicManager) return "None";
            
            auto songInfo = musicManager->getSongInfoObject(songID);
            if (songInfo) {
                auto downloadedPath = musicManager->pathForSong(songID);
                return downloadedPath;
            }
        }
        return "None";
    }

    bool init(LevelEditorLayer* editorLayer){
        if (!EditorUI::init(editorLayer)) return false;
        return true;
    }
};


#include <Geode/modify/SetupAudioLineGuidePopup.hpp>
class $modify(MyLayer, SetupAudioLineGuidePopup){
    void buttonClicked(cocos2d::CCObject* sender){
        std::string fileSongPath = MyEditorUILayer::getSongFilePath();

        log::debug("fileSongPath: {}", fileSongPath);

        if (fileSongPath.empty() || fileSongPath == "None"){
            FLAlertLayer::create(
                    "Error",
                    "Coud not find song path.\nIs your song downloaded and from newgrounds?",
                    "OK"
                )->show();
            return;
        }
        
        //no errors please
        auto BPMValueObject = static_cast<CCTextInputNode*>(this->m_inputNodes->objectForKey(498));
        if (! BPMValueObject) return;

        auto BPMTextField = BPMValueObject->m_textField;
        if (! BPMTextField) return;

        int songBPM = static_cast<int>(std::round(detectBPM(fileSongPath)));

        BPMTextField->setString(std::to_string(songBPM).c_str());
        log::debug("BPM: {}", songBPM);

        //Adds the bpm to the trigger
    }

    bool init(AudioLineGuideGameObject* p0, cocos2d::CCArray* p1){
        if (! SetupAudioLineGuidePopup::init(p0, p1)) return false;

        auto buttonSprite = ButtonSprite::create("Find BPM"); 
        auto findBPMButton = CCMenuItemSpriteExtra::create(
            buttonSprite,
            this,
            menu_selector(MyLayer::buttonClicked)
        );

        m_buttonMenu->addChild(findBPMButton);

        findBPMButton->setPosition({-80.5, 117});
        buttonSprite->setScale(0.45f);

        //makes button

        return true;
    }
};

//Dm me if you found any issues: Games.GG