package mode;

import com.lge.cmuteam3.client.ui.UiController;

import java.awt.*;

public abstract class BaseMode implements Mode {
    private final UiController uiController;

    BaseMode(UiController uiController) {
        this.uiController = uiController;
    }

    public UiController getUiController() {
        return uiController;
    }

    void appendUiLog(String message) {
        EventQueue.invokeLater(() -> {
            uiController.appendLog(message);
        });
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

    @Override
    public boolean needTransferSocket() {
        return true;
    }
}
