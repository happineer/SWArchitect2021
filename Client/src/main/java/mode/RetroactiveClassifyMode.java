package mode;

import com.lge.cmuteam3.client.FileProperties;
import com.lge.cmuteam3.client.network.ScpProxy;
import com.lge.cmuteam3.client.ui.LoadingDialog;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.*;
import java.io.File;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class RetroactiveClassifyMode extends BaseMode {
    private static final Logger LOG = LoggerFactory.getLogger(RetroactiveClassifyMode.class);

    RetroactiveClassifyMode(UiController uiController) {
        super(uiController);
    }

    @Override
    public String getModeName() {
        return "RetroactiveClassify";
    }

    @Override
    public void start() {
        super.start();

        FileProperties prop = FileProperties.getInstance();
        String sourceFolder = prop.getProperty("server.ssh.unknownsFolder");
        String targetFolder = prop.getProperty("client.ssh.unknownsFolder");

        LOG.info("sourceFolder:" + sourceFolder + " targetFolder:" + targetFolder);

        File file = new File(targetFolder);
        if (!file.exists()) {
            boolean mkdir = file.mkdir();
            LOG.info("Create target folder:" + mkdir);
        } else if (file.isFile()) {
            String message = "The target is not a folder. see the client.properties file.";
            alertDialog(message);
            appendUiLog(message);
            ModeManager.getInstance().onUiStop(this);
            return;
        }

        EventQueue.invokeLater(() -> {
            LoadingDialog dialog = new LoadingDialog(getUiController().getMainFrame(), getModeName());

            Executor executors = Executors.newSingleThreadExecutor();
            executors.execute(() -> {
                long start = System.currentTimeMillis();

                ScpProxy scpProxy = new ScpProxy();
                boolean result = scpProxy.receiveFolder(sourceFolder, targetFolder);

                String msg = "Success!";
                if (!result) {
                    msg = "Unknown image download failed.";
                }
                disposeAndShowDialog(dialog, msg, System.currentTimeMillis() - start);

                ModeManager.getInstance().onUiStop(this);
            });
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
    public boolean needTransferSocket() {
        return false;
    }

    @Override
    public RunningButtonMode getRunningButtonMode() {
        return RunningButtonMode.DISABLE_ALL;
    }
}
