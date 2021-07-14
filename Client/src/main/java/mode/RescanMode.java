package mode;

import com.lge.cmuteam3.client.Constants;
import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import java.awt.*;

public class RescanMode extends BaseMode {
    private static final Logger LOG = LoggerFactory.getLogger(RescanMode.class);

    RescanMode(UiController uiController) {
        super(uiController);
    }

    @Override
    public String getModeName() {
        return "Rescan";
    }

    @Override
    public void start() {
        super.start();
        EventQueue.invokeLater(() -> {
            Object[] options = {"Yes, please", "No way!"};
            int n = JOptionPane.showOptionDialog(getUiController().getMainFrame(),
                    "Rescan mode runs in the background and the server stops for a little while. do you want to continue?",
                    "Rescan",
                    JOptionPane.YES_NO_OPTION,
                    JOptionPane.QUESTION_MESSAGE,
                    null,   // icon
                    options,//the titles of buttons
                    options[0]); //default button title

            LOG.info("answer : yes");
            if (n == 0) {
                appendUiLog("Rescan requested!");
                appendUiLog(" ** Rescan mode runs in the background and the server stops for a little while.");
                NetworkManager networkManager = NetworkManager.getInstance();
                networkManager.controlNano(Constants.CONTROL_TYPE_NORMAL, Constants.CONTROL_VALUE_RESCAN);

                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    LOG.info("sleep Interrupted");
                }

                // Disconnect
                networkManager.disconnect(Constants.CONNECTION_STATE_RESCANNING);
                
            } else {
                appendUiLog("Rescan canceled.");
                ModeManager.getInstance().onUiStop(this);
            }
        });
    }

    @Override
    public boolean needTransferSocket() {
        return true;
    }

    @Override
    public RunningButtonMode getRunningButtonMode() {
        return RunningButtonMode.DISABLE_ALL;
    }
}
