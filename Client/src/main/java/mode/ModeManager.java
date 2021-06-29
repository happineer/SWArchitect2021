package mode;

import java.util.ArrayList;
import java.util.List;

public class ModeManager {

	private static ModeManager instance;
	private ArrayList<Mode> modeList = new ArrayList<>();
	private Mode currentMode;
	
	private ModeManager() {
		modeList.add(new TestRunMode());
		modeList.add(new RunMode());
		modeList.add(new TestAccuracyMode());
		currentMode = null;
	}
	
	public static ModeManager getInstance() {
		if (instance == null) {
			instance = new ModeManager();
		}
		return instance;
	}
	
	public void startMode(Mode mode) {
		currentMode = mode;
		mode.start();
	}
	
	public void stopMode() {
		if (currentMode != null) {
			currentMode.stop();
		}
		currentMode = null;
	}
	
	public Mode getCurrentMode(Mode mode) {
		return currentMode;
	}
	
	public List<Mode> getModeList() {
		return modeList;
	}
	
}
