package mode;

import com.lge.cmuteam3.client.network.ScpProxy;
import com.lge.cmuteam3.client.ui.LoadingDialog;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

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
        appendUiLog("Rescan Mode start");
        EventQueue.invokeLater(() -> {
            LoadingDialog dialog = new LoadingDialog(getUiController().getMainFrame(), "Rescan");
            rescan(dialog);
        });
    }

    private void rescan(LoadingDialog dialog) {
        Executor executors = Executors.newSingleThreadExecutor();
        executors.execute(() -> {
            LOG.info("rescan!");

            long start = System.currentTimeMillis();

            ScpProxy scpProxy = new ScpProxy();
            String message = "Rescan Success.";
            if (!scpProxy.rescan()) {
                message = "Rescan failed.";
            }

            disposeAndShowDialog(dialog, message, System.currentTimeMillis() - start);
        });
    }

    private void disposeAndShowDialog(LoadingDialog dialog, String message, long duration) {
        dialog.dispose();

        SimpleDateFormat dateFormat = new SimpleDateFormat("mm:ss.SSS");
        String formattedDuration = dateFormat.format(new Date(duration));
        appendUiLog(message + " Execution time: " + formattedDuration);
        alertDialog(message);
    }

    @Override
    public void stop() {
    }

    @Override
    public boolean needTransferSocket() {
        return false;
    }

}
