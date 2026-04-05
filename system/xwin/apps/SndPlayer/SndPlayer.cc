#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <WidgetEx/FileDialog.h>
#include <WidgetEx/Menubar.h>
#include <Widget/Container.h>

#include <x++/X.h>
#include <unistd.h>
#include <font/font.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/keydef.h>
#include <ewoksys/vfs.h>
#include <ewoksys/proc.h>
#include <ewoksys/timer.h>
#include <ewoksys/ipc.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <fcntl.h>

// Include AudioPlayer class
#include "AudioPlayer.h"

using namespace Ewok;

class SpectrumView : public Widget {
public:
    static const int BARS = 32;

    typedef void (*StateChangeCallback)(void* userData);
    typedef void (*TimeUpdateCallback)(void* userData);
    void setPlayer(AudioPlayer* p) { player = p; }
    void setStateChangeCallback(StateChangeCallback cb, void* ud) {
        stateChangeCb = cb;
        stateChangeUserData = ud;
    }
    void setTimeUpdateCallback(TimeUpdateCallback cb, void* ud) {
        timeUpdateCb = cb;
        timeUpdateUserData = ud;
    }

protected:
    float magnitudes[BARS];
    float targetMagnitudes[BARS];
    uint32_t barColors[BARS];
    AudioPlayer* player;
    StateChangeCallback stateChangeCb;
    void* stateChangeUserData;
    TimeUpdateCallback timeUpdateCb;
    void* timeUpdateUserData;

public:
    SpectrumView() {
        player = NULL;
        stateChangeCb = NULL;
        stateChangeUserData = NULL;
        timeUpdateCb = NULL;
        timeUpdateUserData = NULL;
        for (int i = 0; i < BARS; i++) {
            magnitudes[i] = 0;
            targetMagnitudes[i] = 0;
            barColors[i] = 0xFF00FF00;
        }
    }

    void updateSpectrum(const int16_t* samples, int count, int channels) {
        if (count <= 0) return;

        const int N = 128;
        float fftBuf[N];
        float magnitudesTemp[N/2];

        for (int i = 0; i < N; i++) {
            int idx = (i * count) / N;
            if (idx >= count) idx = count - 1;
            float sample = (float)samples[idx * channels] / 32768.0f;
            float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (N - 1)));
            fftBuf[i] = sample * window;
        }

        for (int k = 0; k < N/2; k++) {
            float real = 0, imag = 0;
            for (int n = 0; n < N; n++) {
                float angle = -2.0f * M_PI * k * n / N;
                real += fftBuf[n] * cosf(angle);
                imag += fftBuf[n] * sinf(angle);
            }
            magnitudesTemp[k] = sqrtf(real * real + imag * imag) / N;
        }

        int barsPerGroup = (N/2) / BARS;
        if (barsPerGroup < 1) barsPerGroup = 1;

        for (int i = 0; i < BARS; i++) {
            float sum = 0;
            int cnt = 0;
            for (int j = 0; j < barsPerGroup; j++) {
                int idx = i * barsPerGroup + j;
                if (idx < N/2) {
                    sum += magnitudesTemp[idx];
                    cnt++;
                }
            }
            if (cnt > 0) {
                targetMagnitudes[i] = (sum / cnt) * 15.0f;
            }
            if (targetMagnitudes[i] > 1.0f) targetMagnitudes[i] = 1.0f;
            if (targetMagnitudes[i] < 0) targetMagnitudes[i] = 0;
        }
    }

    void onTimer(uint32_t timerFPS, uint32_t timerStep) {
        if (player != NULL && player->isPlaying()) {
            player->decodeFrame();
            updateSpectrum(player->getSampleBuf(), player->getSimples(), player->getChannels());

            if (player->isEof()) {
                player->pause();
                if (stateChangeCb) stateChangeCb(stateChangeUserData);
            }
        }

        // Update time label during playback
        if (player != NULL && player->isLoaded() && timeUpdateCb) {
            timeUpdateCb(timeUpdateUserData);
        }

        for (int i = 0; i < BARS; i++) {
            float diff = targetMagnitudes[i] - magnitudes[i];
            if (diff > 0.01f)
                magnitudes[i] += diff * 0.3f;
            else if (diff < -0.01f)
                magnitudes[i] += diff * 0.5f;

            if (magnitudes[i] < 0) magnitudes[i] = 0;
            if (magnitudes[i] > 1) magnitudes[i] = 1;

            uint8_t g = (uint8_t)(magnitudes[i] * 200 + 55);
            uint8_t r = (uint8_t)(magnitudes[i] * 255);
            uint8_t b = (uint8_t)(100 + magnitudes[i] * 100);
            barColors[i] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
        update();
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
        (void)theme;
        graph_fill(g, rect.x, rect.y, rect.w, rect.h, 0xFF1a1a2e);

        int barW = rect.w / BARS;
        if (barW < 2) barW = 2;
        int gap = 1;

        for (int i = 0; i < BARS; i++) {
            int barHeight = (int)(magnitudes[i] * (rect.h - 4));
            if (barHeight < 2) barHeight = 2;

            int bx = rect.x + i * barW + gap/2;
            int by = rect.y + rect.h - barHeight - 2;

            graph_fill(g, bx, by, barW - gap, barHeight, barColors[i]);

            if (barHeight > 4) {
                uint32_t highlightColor = 0xFFFFFFFF;
                graph_fill(g, bx, by, barW - gap, 2, highlightColor);
            }
        }

        uint32_t borderColor = 0xFF444466;
        graph_line(g, rect.x, rect.y, rect.x + rect.w - 1, rect.y, borderColor);
        graph_line(g, rect.x + rect.w - 1, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1, borderColor);
        graph_line(g, rect.x + rect.w - 1, rect.y + rect.h - 1, rect.x, rect.y + rect.h - 1, borderColor);
        graph_line(g, rect.x, rect.y + rect.h - 1, rect.x, rect.y, borderColor);
    }
};

class ProgressBar : public Widget {
public:
    typedef void (*SeekCallback)(void* userData, float progress);

    ProgressBar() {
        progress = 0.0f;
        seekCb = NULL;
        seekUserData = NULL;
        dragging = false;
    }

    void setProgress(float p) {
        if (p < 0.0f) p = 0.0f;
        if (p > 1.0f) p = 1.0f;
        progress = p;
        update();
    }

    float getProgress() { return progress; }

    void setSeekCallback(SeekCallback cb, void* ud) {
        seekCb = cb;
        seekUserData = ud;
    }

protected:
    float progress;
    SeekCallback seekCb;
    void* seekUserData;
    bool dragging;

    void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
        (void)theme;

        // Background
        graph_fill(g, rect.x, rect.y, rect.w, rect.h, 0xFF333344);

        // Progress fill
        int fillWidth = (int)(rect.w * progress);
        if (fillWidth > 0) {
            graph_fill(g, rect.x, rect.y, fillWidth, rect.h, 0xFF00AAFF);
        }

        // Border
        graph_line(g, rect.x, rect.y, rect.x + rect.w - 1, rect.y, 0xFF555566);
        graph_line(g, rect.x + rect.w - 1, rect.y, rect.x + rect.w - 1, rect.y + rect.h - 1, 0xFF555566);
        graph_line(g, rect.x + rect.w - 1, rect.y + rect.h - 1, rect.x, rect.y + rect.h - 1, 0xFF555566);
        graph_line(g, rect.x, rect.y + rect.h - 1, rect.x, rect.y, 0xFF555566);
    }

    bool onMouse(xevent_t* ev) {
        if (ev->type == XEVT_MOUSE) {
            grect_t r = area;
            int mx = ev->value.mouse.x;
            int my = ev->value.mouse.y;

            // Check if mouse is within widget
            if (mx < r.x || mx >= r.x + r.w || my < r.y || my >= r.y + r.h) {
                return false;
            }

            if (ev->state == MOUSE_STATE_DOWN) {
                dragging = true;
                updateProgressFromMouse(mx, r);
                return true;
            } else if (ev->state == MOUSE_STATE_DRAG) {
                if (dragging) {
                    updateProgressFromMouse(mx, r);
                    return true;
                }
            } else if (ev->state == MOUSE_STATE_UP) {
                if (dragging) {
                    dragging = false;
                    updateProgressFromMouse(mx, r);
                    return true;
                }
            }
        }
        return false;
    }

    void updateProgressFromMouse(int mx, const grect_t& r) {
        int relX = mx - r.x;
        float newProgress = (float)relX / r.w;
        if (newProgress < 0.0f) newProgress = 0.0f;
        if (newProgress > 1.0f) newProgress = 1.0f;
        progress = newProgress;
        update();
        if (seekCb) {
            seekCb(seekUserData, progress);
        }
    }
};

class SoundPlayerWin : public WidgetWin {
    FileDialog fdialog;
    AudioPlayer* player;
    SpectrumView* spectrum;
    LabelButton* playBtn;
    Label* timeLabel;
    ProgressBar* progressBar;

protected:
    void onDialoged(XWin* from, int res, void* arg) {
        if (res == Dialog::RES_OK && from == &fdialog) {
            string path = fdialog.getResult();
            if (path.length() > 0) {
                loadAndPlay(path.c_str());
            }
        }
    }

public:
    SoundPlayerWin() {
        player = new AudioPlayer();
        spectrum = NULL;
        playBtn = NULL;
        timeLabel = NULL;
        progressBar = NULL;
    }

    ~SoundPlayerWin() {
        delete player;
    }

    void setSpectrum(SpectrumView* s) { spectrum = s; }
    void setPlayBtn(LabelButton* b) { playBtn = b; }
    void setTimeLabel(Label* l) { timeLabel = l; }
    void setProgressBar(ProgressBar* p) { progressBar = p; }

    void updateProgressBar() {
        if (progressBar && player->isLoaded() && player->getTotalMs() > 0) {
            float progress = (float)player->getCurrentMs() / player->getTotalMs();
            progressBar->setProgress(progress);
        }
    }

    void loadAndPlay(const char* path) {
        if (player->load(path, "/dev/sound0")) {
            player->play();
            updatePlayBtn();
            updateTimeLabel();
        }
    }

    void updatePlayBtn() {
        if (playBtn && player->isLoaded()) {
            if (player->isPlaying()) {
                playBtn->setLabel("||");
            } else {
                playBtn->setLabel(">");
            }
        }
    }

    void updateTimeLabel() {
        if (timeLabel) {
            uint32_t cur = player->getCurrentMs();
            uint32_t tot = player->getTotalMs();
            char buf[64];
            snprintf(buf, sizeof(buf)-1, "%02d:%02d / %02d:%02d",
                cur/60000, (cur/1000)%60,
                tot/60000, (tot/1000)%60);
            timeLabel->setLabel(buf);
        }
    }

    void togglePlay() {
        if (!player->isLoaded()) return;

        if (player->isPlaying()) {
            player->pause();
        } else if (player->isEof()) {
            player->replay("/dev/sound0");
        } else {
            player->play();
        }
        updatePlayBtn();
    }

    AudioPlayer* getPlayer() { return player; }
    FileDialog* getFileDialog() { return &fdialog; }
};

static void onOpenFunc(MenuItem* it, void* p) {
    SoundPlayerWin* win = (SoundPlayerWin*)p;
    win->getFileDialog()->popup(win, 0, 0, "files", XWIN_STYLE_NORMAL);
}

static void onPlayStateChange(void* userData) {
    SoundPlayerWin* win = (SoundPlayerWin*)userData;
    win->updatePlayBtn();
}

static void onTimeUpdate(void* userData) {
    SoundPlayerWin* win = (SoundPlayerWin*)userData;
    win->updateTimeLabel();
    win->updateProgressBar();
}

static void onPlayBtnClick(Widget* wd, xevent_t* evt, void* arg) {
    if(evt->type != XEVT_MOUSE || evt->state != MOUSE_STATE_CLICK)
        return;
    SoundPlayerWin* win = (SoundPlayerWin*)arg;
    win->togglePlay();
}

int main(int argc, char** argv) {
    X x;
    SoundPlayerWin win;

    RootWidget* root = new RootWidget();
    win.setRoot(root);
    root->setType(Container::VERTICLE);

    Menubar* menubar = new Menubar();
    root->add(menubar);
    menubar->fix(0, 24);
    menubar->setItemSize(50);
    menubar->add(0, "Open", NULL, NULL, onOpenFunc, &win);

    SpectrumView* spectrum = new SpectrumView();
    spectrum->setPlayer(win.getPlayer());
    spectrum->setStateChangeCallback(onPlayStateChange, &win);
    spectrum->setTimeUpdateCallback(onTimeUpdate, &win);
    root->add(spectrum);
    win.setSpectrum(spectrum);

    // Progress bar
    ProgressBar* progressBar = new ProgressBar();
    progressBar->fix(0, 8);
    root->add(progressBar);
    win.setProgressBar(progressBar);

    Container* controls = new Container();
    controls->setType(Container::HORIZONTAL);
    root->add(controls);
    controls->fix(0, 24);

    LabelButton* playBtn = new LabelButton(">");
    playBtn->fix(40, 0);
    playBtn->setEventFunc(onPlayBtnClick, &win);
    controls->add(playBtn);
    win.setPlayBtn(playBtn);

    Label* timeLabel = new Label("00:00 / 00:00");
    timeLabel->fix(140, 0);
    controls->add(timeLabel);
    win.setTimeLabel(timeLabel);

    win.open(&x, -1, -1, -1, 300, 140, "SndPlayer", XWIN_STYLE_NORMAL | XWIN_STYLE_NO_BG_EFFECT, true);
    win.setTimer(120);

    if (argc >= 2) {
        win.loadAndPlay(argv[1]);
        win.getPlayer()->play();
    }

    widgetXRun(&x, &win);

    win.getPlayer()->stop();
    return 0;
}
