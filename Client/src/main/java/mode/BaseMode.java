package mode;

import com.lge.cmuteam3.client.ui.UiController;

import javax.swing.*;
import java.awt.*;

public abstract class BaseMode implements Mode {
    private final UiController uiController;
    private boolean isRunning = false;

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

    protected void alertDialog(String message) {
        JFrame frame = getUiController().getMainFrame();
        JOptionPane.showMessageDialog(frame, message);
    }

    @Override
    public RunningButtonMode getRunningButtonMode() {
        return RunningButtonMode.ENABLE_ALL;
    }

    protected void setRunning(boolean isRunning) {
        this.isRunning = isRunning;
    }

    @Override
    public boolean isRunning() {
        return isRunning;
    }

    @Override
    public void start() {
        setRunning(true);
        appendUiLog(getModeName() + " Mode start");
    }

    @Override
    public void stop() {
        setRunning(false);
        appendUiLog(getModeName() + " Mode stop");
    }
    
    @Override
    public boolean needRestore() {
        return false;
    }
}
