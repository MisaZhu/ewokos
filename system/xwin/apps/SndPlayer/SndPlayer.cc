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

protected:
    float magnitudes[BARS];
    float targetMagnitudes[BARS];
    uint32_t barColors[BARS];

public:
    SpectrumView() {
        for (int i = 0; i < BARS; i++) {
            magnitudes[i] = 0;
            targetMagnitudes[i] = 0;
            barColors[i] = 0xFF00FF00;
        }
    }

    void updateSpectrum(const int16_t* samples, int count, int channels) {
        if (samples == NULL || count <= 0 || channels <= 0) return;

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

    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        (void)timerFPS;
        (void)timerSteps;
        bool changed = false;
        for (int i = 0; i < BARS; i++) {
            float oldMagnitude = magnitudes[i];
            uint32_t oldColor = barColors[i];
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

            if (fabsf(magnitudes[i] - oldMagnitude) > 0.001f || barColors[i] != oldColor) {
                changed = true;
            }
        }
        if (changed) {
            update();
        }
    }

protected:
    void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
        (void)theme;
        graph_fill_rect(g, rect.x, rect.y, rect.w, rect.h, 0xFF1a1a2e);

        int barW = rect.w / BARS;
        if (barW < 2) barW = 2;
        int gap = 1;

        for (int i = 0; i < BARS; i++) {
            int barHeight = (int)(magnitudes[i] * (rect.h - 4));
            if (barHeight < 2) barHeight = 2;

            int bx = rect.x + i * barW + gap/2;
            int by = rect.y + rect.h - barHeight - 2;

            graph_fill_rect(g, bx, by, barW - gap, barHeight, barColors[i]);

            if (barHeight > 4) {
                uint32_t highlightColor = 0xFFFFFFFF;
                graph_fill_rect(g, bx, by, barW - gap, 2, highlightColor);
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
        if (fabsf(progress - p) < 0.001f) {
            return;
        }
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
        graph_fill_rect(g, rect.x, rect.y, rect.w, rect.h, 0xFF333344);

        // Progress fill
        int fillWidth = (int)(rect.w * progress);
        if (fillWidth > 0) {
            graph_fill_rect(g, rect.x, rect.y, fillWidth, rect.h, 0xFF00AAFF);
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
        if (fabsf(progress - newProgress) < 0.001f) {
            return;
        }
        progress = newProgress;
        update();
        if (seekCb) {
            seekCb(seekUserData, progress);
        }
    }
};

class SoundPlayerWin : public WidgetWin {
    static const int SNAPSHOT_SAMPLE_CAP = 2304;

    enum PlaybackCommand {
        CMD_NONE,
        CMD_LOAD,
        CMD_TOGGLE,
        CMD_STOP
    };

    struct PlaybackSnapshot {
        bool loaded;
        bool playing;
        bool eof;
        bool writeFailed;
        uint32_t currentMs;
        uint32_t totalMs;
        int channels;
        int sampleCount;
        int16_t samples[SNAPSHOT_SAMPLE_CAP];
    };

    FileDialog fdialog;
    AudioPlayer* player;
    SpectrumView* spectrum;
    LabelButton* playBtn;
    Label* timeLabel;
    ProgressBar* progressBar;
    pthread_t playbackTid;
    pthread_mutex_t playbackLock;
    bool playbackThreadCreated;
    bool playbackThreadExit;
    PlaybackCommand pendingCommand;
    char pendingPath[FS_FULL_NAME_MAX + 1];
    PlaybackSnapshot snapshot;
    bool playBtnShowsPause;
    bool playBtnLabelValid;
    char lastTimeText[64];
    bool timeLabelValid;

    static void* playbackThreadEntry(void* arg) {
        ((SoundPlayerWin*)arg)->playbackLoop();
        return NULL;
    }

    void playbackLoop() {
        char localPath[FS_FULL_NAME_MAX + 1];
        localPath[0] = 0;

        while (true) {
            PlaybackCommand cmd = CMD_NONE;

            pthread_mutex_lock(&playbackLock);
            if (playbackThreadExit) {
                pthread_mutex_unlock(&playbackLock);
                break;
            }
            cmd = pendingCommand;
            if (cmd == CMD_LOAD) {
                strncpy(localPath, pendingPath, sizeof(localPath) - 1);
                localPath[sizeof(localPath) - 1] = 0;
            }
            pendingCommand = CMD_NONE;
            pthread_mutex_unlock(&playbackLock);

            if (cmd == CMD_LOAD) {
                if (player->load(localPath, "/dev/sound0")) {
                    player->play();
                }
                syncSnapshot();
                continue;
            } else if (cmd == CMD_TOGGLE) {
                if (player->isLoaded()) {
                    if (player->isWriteFailed()) {
                        player->reopenDevice("/dev/sound0");
                        player->play();
                    } else if (player->isEof()) {
                        player->replay("/dev/sound0");
                        player->play();
                    } else if (player->isPlaying()) {
                        player->pause();
                    } else {
                        player->play();
                    }
                }
                syncSnapshot();
                continue;
            } else if (cmd == CMD_STOP) {
                player->stop();
                syncSnapshot();
                proc_usleep(5000);
                continue;
            }

            if (player->isPlaying()) {
                player->decodeFrame();
                syncSnapshot();
                continue;
            }

            syncSnapshot();
            proc_usleep(5000);
        }

        player->stop();
        syncSnapshot();
    }

    void syncSnapshot() {
        PlaybackSnapshot next;
        next.loaded = player->isLoaded();
        next.playing = player->isPlaying();
        next.eof = player->isEof();
        next.writeFailed = player->isWriteFailed();
        next.currentMs = player->getCurrentMs();
        next.totalMs = player->getTotalMs();
        next.channels = player->getChannels();
        next.sampleCount = player->getSimples();
        if (next.channels <= 0) {
            next.channels = 2;
        }
        if (next.sampleCount < 0) {
            next.sampleCount = 0;
        }
        if (next.sampleCount > (SNAPSHOT_SAMPLE_CAP / next.channels)) {
            next.sampleCount = SNAPSHOT_SAMPLE_CAP / next.channels;
        }

        int16_t* src = player->getSampleBuf();
        int totalSampleValues = next.sampleCount * next.channels;
        if (src != NULL && totalSampleValues > 0) {
            memcpy(next.samples, src, totalSampleValues * (int)sizeof(int16_t));
        } else {
            memset(next.samples, 0, sizeof(next.samples));
            next.sampleCount = 0;
        }

        pthread_mutex_lock(&playbackLock);
        snapshot = next;
        pthread_mutex_unlock(&playbackLock);
    }

    void copySnapshot(PlaybackSnapshot* out) {
        pthread_mutex_lock(&playbackLock);
        *out = snapshot;
        pthread_mutex_unlock(&playbackLock);
    }

    void queueLoad(const char* path) {
        pthread_mutex_lock(&playbackLock);
        strncpy(pendingPath, path, sizeof(pendingPath) - 1);
        pendingPath[sizeof(pendingPath) - 1] = 0;
        pendingCommand = CMD_LOAD;
        pthread_mutex_unlock(&playbackLock);
    }

    void queueToggle() {
        pthread_mutex_lock(&playbackLock);
        pendingCommand = CMD_TOGGLE;
        pthread_mutex_unlock(&playbackLock);
    }

    void queueStop() {
        pthread_mutex_lock(&playbackLock);
        pendingCommand = CMD_STOP;
        pthread_mutex_unlock(&playbackLock);
    }

    void stopPlaybackThread() {
        if (!playbackThreadCreated) {
            return;
        }

        pthread_mutex_lock(&playbackLock);
        playbackThreadExit = true;
        pthread_mutex_unlock(&playbackLock);

        pthread_join(playbackTid, NULL);
        playbackThreadCreated = false;
    }

    void updatePlayBtn(const PlaybackSnapshot& state) {
        if (playBtn) {
            bool showPause = state.playing && !state.writeFailed;
            if (!playBtnLabelValid || playBtnShowsPause != showPause) {
                playBtn->setLabel(showPause ? "||" : ">");
                playBtnShowsPause = showPause;
                playBtnLabelValid = true;
            }
        }
    }

    void updateTimeLabel(const PlaybackSnapshot& state) {
        if (timeLabel) {
            uint32_t cur = state.currentMs;
            uint32_t tot = state.totalMs;
            char buf[64];
            snprintf(buf, sizeof(buf)-1, "%02d:%02d / %02d:%02d",
                cur/60000, (cur/1000)%60,
                tot/60000, (tot/1000)%60);
            buf[sizeof(buf)-1] = 0;
            if (!timeLabelValid || strcmp(lastTimeText, buf) != 0) {
                timeLabel->setLabel(buf);
                strncpy(lastTimeText, buf, sizeof(lastTimeText) - 1);
                lastTimeText[sizeof(lastTimeText) - 1] = 0;
                timeLabelValid = true;
            }
        }
    }

    void updateProgressBar(const PlaybackSnapshot& state) {
        if (progressBar && state.loaded && state.totalMs > 0) {
            float progress = (float)state.currentMs / state.totalMs;
            progressBar->setProgress(progress);
        }
    }

protected:
    void onTimer(uint32_t timerFPS, uint32_t timerSteps) {
        (void)timerFPS;
        (void)timerSteps;

        if (!playbackThreadCreated && player->isPlaying()) {
            player->decodeFrame();
            syncSnapshot();
        }

        PlaybackSnapshot state;
        copySnapshot(&state);

        if (spectrum != NULL && state.sampleCount > 0 && state.channels > 0) {
            spectrum->updateSpectrum(state.samples, state.sampleCount, state.channels);
        }

        updatePlayBtn(state);
        updateTimeLabel(state);
        updateProgressBar(state);
    }

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
        playbackThreadCreated = false;
        playbackThreadExit = false;
        pendingCommand = CMD_NONE;
        pendingPath[0] = 0;
        memset(&snapshot, 0, sizeof(snapshot));
        playBtnShowsPause = false;
        playBtnLabelValid = false;
        lastTimeText[0] = 0;
        timeLabelValid = false;
        pthread_mutex_init(&playbackLock, NULL);
        if (pthread_create(&playbackTid, NULL, playbackThreadEntry, this) == 0) {
            playbackThreadCreated = true;
        }
    }

    ~SoundPlayerWin() {
        stopPlaybackThread();
        pthread_mutex_destroy(&playbackLock);
        delete player;
    }

    void setSpectrum(SpectrumView* s) { spectrum = s; }
    void setPlayBtn(LabelButton* b) { playBtn = b; }
    void setTimeLabel(Label* l) { timeLabel = l; }
    void setProgressBar(ProgressBar* p) { progressBar = p; }

    void loadAndPlay(const char* path) {
        if (!playbackThreadCreated) {
            if (player->load(path, "/dev/sound0")) {
                player->play();
            }
            syncSnapshot();
            return;
        }
        queueLoad(path);
    }

    void togglePlay() {
        if (!playbackThreadCreated) {
            if (!player->isLoaded()) return;

            if (player->isWriteFailed()) {
                player->reopenDevice("/dev/sound0");
                player->play();
            } else if (player->isEof()) {
                player->replay("/dev/sound0");
                player->play();
            } else if (player->isPlaying()) {
                player->pause();
            } else {
                player->play();
            }
            syncSnapshot();
            return;
        }
        queueToggle();
    }

    AudioPlayer* getPlayer() { return player; }
    FileDialog* getFileDialog() { return &fdialog; }
    void stopPlayback() {
        if (!playbackThreadCreated) {
            player->stop();
            syncSnapshot();
            return;
        }
        queueStop();
    }
};

static void onOpenFunc(MenuItem* it, void* p) {
    SoundPlayerWin* win = (SoundPlayerWin*)p;
    win->getFileDialog()->popup(win, 0, 0, "files", XWIN_STYLE_NORMAL);
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
    root->setType(Container::VERTICAL);

    Menubar* menubar = new Menubar();
    root->add(menubar);
    menubar->fix(0, 24);
    menubar->setItemSize(50);
    menubar->add(0, "Open", NULL, NULL, onOpenFunc, &win);

    SpectrumView* spectrum = new SpectrumView();
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
    win.setTimer(16);

    if (argc >= 2) {
        win.loadAndPlay(argv[1]);
    }

    widgetXRun(&x, &win);

    win.stopPlayback();
    return 0;
}
