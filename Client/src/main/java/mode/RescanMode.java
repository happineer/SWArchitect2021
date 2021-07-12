package mode;

import com.lge.cmuteam3.client.network.ScpProxy;
import com.lge.cmuteam3.client.ui.LoadingDialog;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.awt.*;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;

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
            LoadingDialog dialog = new LoadingDialog(getUiController().getMainFrame(), "Rescan");
            rescan(dialog);
        });
    }

    private void rescan(LoadingDialog dialog) {
        ExecutorService service = Executors.newFixedThreadPool(1);

        AtomicBoolean isCanceled = new AtomicBoolean(false);
        long start = System.currentTimeMillis();
        Future<String> futureResult = service.submit(() -> {

            LOG.info("rescan!");

            ScpProxy scpProxy = new ScpProxy();
            String message = "Rescan Success.";
            if (!scpProxy.rescan()) {
                message = "Rescan failed.";
            }

            if (!isCanceled.get())
                disposeAndShowDialog(dialog, message, System.currentTimeMillis() - start);

            return "";
        });

        Executor executors = Executors.newSingleThreadExecutor();
        executors.execute(() -> {
            try {
                futureResult.get(3, TimeUnit.MINUTES);
            } catch (InterruptedException e) {
                isCanceled.set(true);
                disposeAndShowDialog(dialog, "Rescan interrupted.", System.currentTimeMillis() - start);
            } catch (ExecutionException e) {
                isCanceled.set(true);
                disposeAndShowDialog(dialog, "Rescan - Execution Exception.", System.currentTimeMillis() - start);
            } catch (TimeoutException e) {
                isCanceled.set(true);
                disposeAndShowDialog(dialog, "Rescan - Timeout occurred.", System.currentTimeMillis() - start);
            } finally {
                service.shutdown();
                ModeManager.getInstance().onUiStop(this);
            }
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
