package mode;

import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.OnUiEventListener;
import com.lge.cmuteam3.client.ui.UiController;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class ModeManager implements OnUiEventListener {

	private static ModeManager instance;
	private ArrayList<Mode> modeList = new ArrayList<>();
	private Mode currentMode;
	
	private ModeManager() {
		currentMode = null;
	}

	public static ModeManager getInstance() {
		if (instance == null) {
			instance = new ModeManager();
		}
		return instance;
	}

	public void init(UiController uiController) {
		modeList = new ArrayList<>();
		modeList.add(new LearnMode(uiController));
		modeList.add(new RescanMode(uiController));
		modeList.add(new RunMode(uiController));
		modeList.add(new TestRunMode(uiController));
		modeList.add(new TestAccuracyMode(uiController));

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
		ExecutorService exec = Executors.newSingleThreadExecutor();
		exec.submit(() -> {
			if (currentMode != null) {
				currentMode.stop();
			}

			if (mode.needTransferSocket()) {
				if (!NetworkManager.getInstance().isReady()) {
					NetworkManager.getInstance().init();
					return;
				}
			}

			currentMode = mode;
			currentMode.start();
		});
	}

	@Override
	public void onUiStop(Mode mode) {
		if (currentMode != null) {
			currentMode.stop();
		}
		currentMode = null;
	}
}
