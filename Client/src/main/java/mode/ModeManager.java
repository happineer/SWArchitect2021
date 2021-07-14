package mode;

import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.network.OnServerStateListener;
import com.lge.cmuteam3.client.ui.OnUiEventListener;
import com.lge.cmuteam3.client.ui.UiController;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ModeManager implements OnUiEventListener, OnServerStateListener {
	private static final Logger LOG = LoggerFactory.getLogger(ModeManager.class);

	private static ModeManager instance;
	private ArrayList<Mode> modeList = new ArrayList<>();
	private UiController uiController;
	private Mode currentMode;
	
	private ModeManager() {
		currentMode = null;
	}

	public static ModeManager getInstance() {
		if (instance == null) {
			instance = new ModeManager();
			NetworkManager.getInstance().setOnServerStateListener(instance);
		}
		return instance;
	}

	public void init(UiController uiController) {
		this.uiController = uiController;
		modeList = new ArrayList<>();
		modeList.add(new InitMode(uiController));
		modeList.add(new LearnMode(uiController));
		modeList.add(new RescanMode(uiController));
		modeList.add(new RunMode(uiController));
		modeList.add(new TestRunMode(uiController));
		modeList.add(new TestAccuracyMode(uiController));
		modeList.add(new RetroactiveRunMode(uiController));
		modeList.add(new RetroactiveClassifyMode(uiController));

		uiController.setModePanel(modeList, this);
	}

	public Mode getCurrentMode(Mode mode) {
		return currentMode;
	}
	
	public List<Mode> getModeList() {
		return modeList;
	}

	@Override
	public void onUiStart(Mode mode) {
		LOG.info("onUiStop:" + mode.getModeName());
		// !(mode instanceof InitMode) init mode 는 별도의 모드가 아닌 내부 기능으로 분류될 수 있음. 
		if (mode.needTransferSocket() && !NetworkManager.getInstance().isReady()) {
			uiController.appendLog("NANO NOT READY. PRESS INIT");
			return;
		}
		
		ExecutorService exec = Executors.newSingleThreadExecutor();
		exec.submit(() -> {
			if (currentMode != null) {
				currentMode.stop();
				uiController.enableAllControlButtons();
			}

			currentMode = mode;
			uiController.disableControlButtons(currentMode);
			currentMode.start();
		});
	}

	@Override
	public void onUiStop(Mode mode) {
		LOG.info("onUiStop:" + (mode == null? "no mode" : mode.getModeName()));
		if (currentMode != null) {
			currentMode.stop();
		}
		uiController.enableAllControlButtons();
		currentMode = null;
	}

	@Override
	public void onReady() {
		uiController.updateConnectionStatus("Connected");
	}

	@Override
	public void onFail(int serverState) {
		if (serverState == 1)
			uiController.updateConnectionStatus("Reconnecting...");
		else if (serverState == 2)
			uiController.updateConnectionStatus("Failed");
		else if (serverState == 0)
			uiController.updateConnectionStatus("Disconnected");
		onUiStop(currentMode);
	}
}
