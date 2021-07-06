package com.lge.cmuteam3.client.ui;

import java.awt.Dimension;

import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.SwingConstants;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class StatisticsPanel extends JPanel {
	private static final int TEXTFIELD_WIDTH = 150;

	private static final Logger LOG = LoggerFactory.getLogger(StatisticsPanel.class);

	private static final long serialVersionUID = -5973217190507273135L;

	private final JLabel framesLabel;
	private final JTextField frameTextField;
	private final JLabel maxLatencyLabel;
	private final JTextField maxLatencyTextField;
	private final JLabel minLatencyLabel;
	private final JTextField minLatencyTextField;
	private final JLabel avrLatencyLabel;
	private final JTextField avrLatencyTextField;
	private final JLabel fullTimeFpsLabel;
	private final JTextField fullTimeFpsTextField;
	private final JLabel fpsLabel;
	private final JTextField fpsTextField;

	public StatisticsPanel() {
		setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));

		framesLabel = new JLabel("Frames");
		framesLabel.setVerticalAlignment(SwingConstants.TOP);
		add(framesLabel);

		frameTextField = new JTextField();
		setCommonStyleTextField(frameTextField);

		minLatencyLabel = new JLabel("Min Latency");
		add(minLatencyLabel);

		minLatencyTextField = new JTextField();
		setCommonStyleTextField(minLatencyTextField);

		maxLatencyLabel = new JLabel("Max Latency");
		add(maxLatencyLabel);

		maxLatencyTextField = new JTextField();
		setCommonStyleTextField(maxLatencyTextField);

		avrLatencyLabel = new JLabel("Avr Latency");
		add(avrLatencyLabel);

		avrLatencyTextField = new JTextField();
		setCommonStyleTextField(avrLatencyTextField);
		
		fullTimeFpsLabel = new JLabel("Full-time FPS");
		add(fullTimeFpsLabel);

		fullTimeFpsTextField = new JTextField();
		setCommonStyleTextField(fullTimeFpsTextField);

		fpsLabel = new JLabel("FPS");
		add(fpsLabel);

		fpsTextField = new JTextField();
		setCommonStyleTextField(fpsTextField);
	}

	private void setCommonStyleTextField(JTextField textField) {
		textField.setEditable(false);
		textField.setText("0");
		textField.setColumns(1);
		setInlineTextField(textField);
		add(textField);
	}

	private void setInlineTextField(JTextField field) {
		Dimension curd = field.getSize();
		Dimension mind = field.getPreferredSize();
		curd.setSize(TEXTFIELD_WIDTH, mind.height);
		field.setMaximumSize(curd);
	}

	public void update(long frame, long max, long min, long avr, double fullTimeFps, double fps) {
		frameTextField.setText(Long.toString(frame));
		maxLatencyTextField.setText(Long.toString(max));
		minLatencyTextField.setText(Long.toString(min));
		avrLatencyTextField.setText(Long.toString(avr));
		fullTimeFpsTextField.setText(String.format("%.4f", fullTimeFps));
		fpsTextField.setText(String.format("%.4f", fps));
	}

	public void update(UiModel uiModel) {
		update(uiModel.getCount(), uiModel.getMax(), uiModel.getMin(), uiModel.getAvr(), uiModel.getFullTimeFps(), uiModel.getFps());
	}

	public void reset() {
		update(0, 0, 0, 0, 0, 0);
	}
}
