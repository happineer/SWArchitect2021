package mode;

import com.lge.cmuteam3.client.FileProperties;
import com.lge.cmuteam3.client.network.ScpProxy;
import com.lge.cmuteam3.client.ui.LeaningModeDialog;
import com.lge.cmuteam3.client.ui.SendProgressDialog;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import javax.swing.filechooser.FileFilter;
import java.awt.*;
import java.io.File;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

public class LearnMode extends BaseMode implements LeaningModeDialog.OnDialogEventListener {
    private static final Logger LOG = LoggerFactory.getLogger(LearnMode.class);

    LearnMode(UiController uiController) {
        super(uiController);
    }

    @Override
    public String getModeName() {
        return "Learn";
    }

    @Override
    public void start() {
        super.start();

        JFrame frame = getUiController().getMainFrame();

        JFileChooser fileChooser = new JFileChooser();
        fileChooser.addChoosableFileFilter(new ImageFilter());
        fileChooser.setAcceptAllFileFilterUsed(false);
        fileChooser.setMultiSelectionEnabled(true);

        EventQueue.invokeLater(() -> {
            int option = fileChooser.showOpenDialog(frame);
            if (option == JFileChooser.APPROVE_OPTION) {
                File[] files = fileChooser.getSelectedFiles();

                LOG.info("File Selected: " + files.length);

                LeaningModeDialog dialog = new LeaningModeDialog(frame, "Add images...", files, this);
                dialog.setLocationRelativeTo(frame);
                dialog.pack();
                dialog.setVisible(true);
            } else {
                LOG.info("Open command canceled");

                ModeManager.getInstance().onUiStop(this);
            }
        });
    }

    @Override
    public boolean needTransferSocket() {
        return false;
    }

    @Override
    public void onDialogCanceled() {
        ModeManager.getInstance().onUiStop(this);
    }

    @Override
    public void onDialogSend(String name, File[] files) {
        LOG.info("Send confirmed! : " + files.length);
        JFrame frame = getUiController().getMainFrame();
        SendProgressDialog sendProgressDialog = new SendProgressDialog(frame, "Sending images...");
        sendImages(sendProgressDialog, name, files);
    }

    private void sendImages(SendProgressDialog sendProgressDialog, String name, File[] files) {
        Executor executors = Executors.newSingleThreadExecutor();
        executors.execute(() -> {
            LOG.info("Execute!");

            ScpProxy scpProxy = new ScpProxy();
            if (!scpProxy.createFolder(name)) {
                appendUiLog("\"" + name + "\" folder creation failed. At first, check if the " + FileProperties.get("client.ssh.keyFilePath") + " file exists, and if not, run the 'ssh-keygen -m PEM' command. And check other ssh properties in 'client.properties' file.");
                alertDialog("folder creation failed.");
                sendProgressDialog.dispose();
                return;
            }
            appendUiLog("name: " + name);

            for (int i = 0; i < files.length; i++) {
                File file = files[i];
                boolean success = scpProxy.sendFile(file, name);
                if (success) {
                    appendUiLog(file + " sent successfully.");
                } else {
                    appendUiLog(file + " sending failed.");
                    alertDialog("Sending images failed.");
                    sendProgressDialog.dispose();
                    return;
                }
                sendProgressDialog.update(100 / files.length * i);
            }

            sendProgressDialog.dispose();
            alertDialog("Sending images completed.");

            ModeManager.getInstance().onUiStop(this);
        });
    }

    static class ImageFilter extends FileFilter {
        public final static String JPEG = "jpeg";
        public final static String JPG = "jpg";
        public final static String GIF = "gif";
        public final static String TIFF = "tiff";
        public final static String TIF = "tif";
        public final static String PNG = "png";

        @Override
        public boolean accept(File f) {
            if (f.isDirectory()) {
                return true;
            }

            String extension = getExtension(f);
            if (extension != null) {
                return extension.equals(TIFF) ||
                        extension.equals(TIF) ||
                        extension.equals(GIF) ||
                        extension.equals(JPEG) ||
                        extension.equals(JPG) ||
                        extension.equals(PNG);
            }
            return false;
        }

        @Override
        public String getDescription() {
            return "Image Only";
        }

        String getExtension(File f) {
            String ext = null;
            String s = f.getName();
            int i = s.lastIndexOf('.');

            if (i > 0 && i < s.length() - 1) {
                ext = s.substring(i + 1).toLowerCase();
            }
            return ext;
        }
    }

    @Override
    public RunningButtonMode getRunningButtonMode() {
        return RunningButtonMode.DISABLE_ALL;
    }
}
