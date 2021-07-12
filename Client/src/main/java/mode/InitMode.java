package mode;

import com.lge.cmuteam3.client.network.NetworkManager;
import com.lge.cmuteam3.client.ui.UiController;

public class InitMode extends BaseMode {
	InitMode(UiController uiController) {
		super(uiController);
	}

	@Override
	public String getModeName() {
		return "Init";
	}
	
	@Override
	public void start() {
		super.start();
		getUiController().updateConnectionStatus("Connecting");
		NetworkManager manager = NetworkManager.getInstance();
		if (manager.isReady()) {
			appendUiLog("Already Initialized");
			return;
		}
		NetworkManager.getInstance().initialize();
	}

	@Override
	public boolean needTransferSocket() {
		return false;
	}

	@Override
	public RunningButtonMode getRunningButtonMode() {
		return RunningButtonMode.ENABLE_ALL;
	}
}
