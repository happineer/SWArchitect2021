package com.lge.cmuteam3.client.ui;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.swing.*;
import javax.swing.border.Border;
import javax.swing.border.CompoundBorder;
import javax.swing.border.EmptyBorder;
import javax.swing.border.LineBorder;
import java.awt.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

public class StatisticsPanel extends JPanel {
	private static final int TEXTFIELD_WIDTH = 220;

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
	private final JLabel elapsedTimeLabel;
	private final JTextField elapsedTimeTextField;
	private final JLabel averageJitterLabel;
	private final JTextField averageJitterTextField;

	public StatisticsPanel() {
		setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));

		framesLabel = new JLabel("Frames");
		framesLabel.setFont(new Font("Courier New", Font.PLAIN, 19));
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

		fpsLabel = new JLabel("Frame rate");
		add(fpsLabel);

		fpsTextField = new JTextField();
		setCommonStyleTextField(fpsTextField);

		fullTimeFpsLabel = new JLabel("Avr Frame rate");
		add(fullTimeFpsLabel);

		fullTimeFpsTextField = new JTextField();
		setCommonStyleTextField(fullTimeFpsTextField);

		averageJitterLabel = new JLabel("Avr Jitter");
		add(averageJitterLabel);

		averageJitterTextField = new JTextField();
		setCommonStyleTextField(averageJitterTextField);

		elapsedTimeLabel = new JLabel("Elapsed Time");
		add(elapsedTimeLabel);
		elapsedTimeTextField = new JTextField();
		setCommonStyleTextField(elapsedTimeTextField);

		Component[] components = getComponents();
		for (Component c : components) {
			c.setFont(new Font("Lucida Sans", Font.PLAIN, 16));
		}
	}

	private void setCommonStyleTextField(JTextField textField) {
		textField.setEditable(false);
		textField.setText("0");
		textField.setColumns(1);
		Border rounded = new LineBorder(new Color(150,150,150), 1, true);
		Border empty = new EmptyBorder(0, 5, 7, 0);
		Border border = new CompoundBorder(empty, textField.getBorder());
		textField.setBorder(border);
		setInlineTextField(textField);
		add(textField);
	}

	private void setInlineTextField(JTextField field) {
		Dimension curd = field.getSize();
		Dimension mind = field.getPreferredSize();
		curd.setSize(TEXTFIELD_WIDTH, mind.height);
		field.setMaximumSize(curd);
	}

	public void update(long frame, long max, long min, long avr, double fullTimeFps, int fps, long elapsedTime, double averageJitter) {
		frameTextField.setText(Long.toString(frame));
		maxLatencyTextField.setText(Long.toString(max));
		minLatencyTextField.setText(Long.toString(min));
		avrLatencyTextField.setText(Long.toString(avr));
		fpsTextField.setText(Integer.toString(fps));
		fullTimeFpsTextField.setText(String.format("%.2f", fullTimeFps));
		averageJitterTextField.setText(String.format("%.2f", averageJitter));

		String dateFormatted = formatTime(elapsedTime);
		elapsedTimeTextField.setText(dateFormatted);
	}

	private String formatTime(long elapsedTime) {
		Date date = new Date(elapsedTime);
		DateFormat formatter = new SimpleDateFormat("HH:mm:ss.SSS");
		formatter.setTimeZone(TimeZone.getTimeZone("UTC"));
		return formatter.format(date);
	}

	public void update(UiModel uiModel) {
		update(uiModel.getCount(), uiModel.getMax(), uiModel.getMin(), uiModel.getAvr(), uiModel.getAverageFps(), uiModel.getFps(), uiModel.getElapsedTime(), uiModel.getAverageJitter());
	}

	public void reset() {
		update(0, 0, 0, 0, 0, 0, 0, 0);
	}
}
