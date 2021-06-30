package mode;

import com.lge.cmuteam3.client.ui.UiController;

public abstract class BaseMode implements Mode {
    private final UiController uiController;

    BaseMode(UiController uiController) {
        this.uiController = uiController;
    }

    void appendUiLog(String message) {
        uiController.appendLog(message);
    }

    /**
     * Clear UI Log view
     */
    void clearUiLog() {
        uiController.clearLog();
    }

    /**
     * Clear and set a message to UI Log view
     * @param message Log message to display on UI
     */
    void setUiLog(String message) {
        uiController.clearLog();
    }
}
