import javax.swing.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.net.*;

public class RobotController {

    static Socket        socket;
    static PrintWriter   writer;
    static BufferedReader reader;
    static boolean       isManual = true;

    static JButton btnForward, btnLeft, btnRight, btnBack, btnStop, btnModeToggle;
    static JLabel  lblFront, lblLeft, lblRight, lblStatus;

    static volatile int distFront = 99, distLeft = 99, distRight = 99;

    // ─────────────────────────────────────────────────────────
    public static void main(String[] args) {
        String ip = JOptionPane.showInputDialog(null,
            "Inserisci IP del Raspberry Pi:", "192.168.1.100");
        if (ip == null || ip.isBlank()) ip = "192.168.1.100";
        ip = ip.trim();

        try {
            socket = new Socket(ip, 5000);
            socket.setSoTimeout(2000);
            writer = new PrintWriter(socket.getOutputStream(), true);
            reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            System.out.println("Connesso a " + ip);
        } catch (Exception ex) {
            JOptionPane.showMessageDialog(null, "Connessione fallita!\n" + ex.getMessage());
            System.exit(1);
        }

        SwingUtilities.invokeLater(RobotController::buildUI);
        Thread t = new Thread(RobotController::readLoop, "sonar-reader");
        t.setDaemon(true);
        t.start();
    }

    // ── UI ────────────────────────────────────────────────────
    static void buildUI() {
        JFrame frame = new JFrame("Spider Robot Controller");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(420, 540);
        frame.setResizable(false);
        frame.getContentPane().setBackground(new Color(20, 20, 30));

        JPanel root = new JPanel(new BorderLayout(10, 10));
        root.setBackground(new Color(20, 20, 30));
        root.setBorder(new EmptyBorder(12, 12, 12, 12));

        JLabel title = new JLabel("SPIDER ROBOT", SwingConstants.CENTER);
        title.setFont(new Font("Monospaced", Font.BOLD, 22));
        title.setForeground(new Color(0, 220, 180));
        root.add(title, BorderLayout.NORTH);

        JPanel center = new JPanel(new GridLayout(2, 1, 8, 8));
        center.setOpaque(false);
        center.add(buildControlPanel());
        center.add(buildSonarPanel());
        root.add(center, BorderLayout.CENTER);

        JPanel bottom = new JPanel(new BorderLayout(6, 6));
        bottom.setOpaque(false);

        btnModeToggle = makeButton("  Modalita: MANUALE", new Color(60, 60, 100));
        btnModeToggle.addActionListener(e -> toggleMode());
        bottom.add(btnModeToggle, BorderLayout.CENTER);

        lblStatus = new JLabel("Connesso", SwingConstants.CENTER);
        lblStatus.setForeground(new Color(0, 220, 120));
        lblStatus.setFont(new Font("Monospaced", Font.PLAIN, 11));
        bottom.add(lblStatus, BorderLayout.SOUTH);

        root.add(bottom, BorderLayout.SOUTH);
        frame.add(root);
        frame.setVisible(true);

        frame.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                int k = e.getKeyCode();
                if (k == KeyEvent.VK_UP    || k == KeyEvent.VK_W) sendCommand("F");
                if (k == KeyEvent.VK_DOWN  || k == KeyEvent.VK_S) sendCommand("B");
                if (k == KeyEvent.VK_LEFT  || k == KeyEvent.VK_A) sendCommand("L");
                if (k == KeyEvent.VK_RIGHT || k == KeyEvent.VK_D) sendCommand("R");
                if (k == KeyEvent.VK_SPACE)                        sendCommand("S");
            }
        });
        frame.setFocusable(true);
    }

    static JPanel buildControlPanel() {
        JPanel p = new JPanel(new GridBagLayout());
        p.setBackground(new Color(28, 28, 42));
        p.setBorder(BorderFactory.createTitledBorder(
            BorderFactory.createLineBorder(new Color(0, 180, 150), 1),
            "Controllo", TitledBorder.LEFT, TitledBorder.TOP,
            new Font("Monospaced", Font.BOLD, 11), new Color(0, 180, 150)));

        GridBagConstraints g = new GridBagConstraints();
        g.insets  = new Insets(4, 4, 4, 4);
        g.fill    = GridBagConstraints.BOTH;
        g.weightx = 1;
        g.weighty = 1;

        btnForward = makeButton("  AVANTI",   new Color(0, 140, 100));
        btnLeft    = makeButton("  SINISTRA", new Color(0, 100, 160));
        btnStop    = makeButton("  STOP",     new Color(180, 50, 50));
        btnRight   = makeButton("  DESTRA",   new Color(0, 100, 160));
        btnBack    = makeButton("  INDIETRO", new Color(100, 80, 0));

        btnForward.addActionListener(e -> sendCommand("F"));
        btnLeft   .addActionListener(e -> sendCommand("L"));
        btnStop   .addActionListener(e -> sendCommand("S"));
        btnRight  .addActionListener(e -> sendCommand("R"));
        btnBack   .addActionListener(e -> sendCommand("B"));

        g.gridx = 1; g.gridy = 0; p.add(btnForward, g);
        g.gridx = 0; g.gridy = 1; p.add(btnLeft,    g);
        g.gridx = 1; g.gridy = 1; p.add(btnStop,    g);
        g.gridx = 2; g.gridy = 1; p.add(btnRight,   g);
        g.gridx = 1; g.gridy = 2; p.add(btnBack,    g);
        return p;
    }

    static JPanel buildSonarPanel() {
        JPanel p = new JPanel(new GridLayout(1, 3, 8, 0));
        p.setBackground(new Color(28, 28, 42));
        p.setBorder(BorderFactory.createTitledBorder(
            BorderFactory.createLineBorder(new Color(0, 180, 150), 1),
            "Sensori ultrasuoni (cm)", TitledBorder.LEFT, TitledBorder.TOP,
            new Font("Monospaced", Font.BOLD, 11), new Color(0, 180, 150)));

        lblLeft  = makeSonarLabel("SX",    99);
        lblFront = makeSonarLabel("FRONT", 99);
        lblRight = makeSonarLabel("DX",    99);

        p.add(lblLeft);
        p.add(lblFront);
        p.add(lblRight);
        return p;
    }

    static JLabel makeSonarLabel(String name, int val) {
        JLabel l = new JLabel(sonarHtml(name, val), SwingConstants.CENTER);
        l.setOpaque(true);
        l.setBackground(new Color(18, 18, 32));
        l.setBorder(BorderFactory.createLineBorder(new Color(40, 40, 60)));
        return l;
    }

    static JButton makeButton(String text, Color bg) {
        JButton b = new JButton(text);
        b.setBackground(bg);
        b.setForeground(Color.WHITE);
        b.setFont(new Font("Monospaced", Font.BOLD, 13));
        b.setFocusPainted(false);
        b.setBorder(BorderFactory.createCompoundBorder(
            BorderFactory.createLineBorder(bg.brighter(), 1),
            new EmptyBorder(8, 12, 8, 12)));
        b.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
        b.addMouseListener(new MouseAdapter() {
            public void mouseEntered(MouseEvent e) { b.setBackground(bg.brighter()); }
            public void mouseExited (MouseEvent e) { b.setBackground(bg); }
        });
        return b;
    }

    // ── Commands ──────────────────────────────────────────────
    static void sendCommand(String cmd) {
        if (writer != null && isManual) {
            writer.println(cmd);
            System.out.println("Sent: " + cmd);
        }
    }

    static void sendRaw(String cmd) {
        if (writer != null) {
            writer.println(cmd);
            System.out.println("Sent raw: " + cmd);
        }
    }

    static void toggleMode() {
        isManual = !isManual;
        if (isManual) {
            btnModeToggle.setText("  Modalita: MANUALE");
            btnModeToggle.setBackground(new Color(60, 60, 100));
            sendRaw("MODE_MANUAL");
        } else {
            btnModeToggle.setText("  Modalita: IA");
            btnModeToggle.setBackground(new Color(120, 40, 120));
            sendRaw("MODE_AI");
        }
        btnForward.setEnabled(isManual);
        btnLeft   .setEnabled(isManual);
        btnRight  .setEnabled(isManual);
        btnBack   .setEnabled(isManual);
        btnStop   .setEnabled(isManual);
    }

    // ── Read loop: receives SONAR pushes from pi_server ───────
    static void readLoop() {
        while (!Thread.currentThread().isInterrupted()) {
            try {
                String line = reader.readLine();
                if (line == null) {
                    // server closed connection
                    SwingUtilities.invokeLater(() -> lblStatus.setText("Disconnesso"));
                    break;
                }
                if (line.startsWith("SONAR:")) {
                    String[] parts = line.substring(6).split(",");
                    if (parts.length == 3) {
                        distFront = Integer.parseInt(parts[0].trim());
                        distLeft  = Integer.parseInt(parts[1].trim());
                        distRight = Integer.parseInt(parts[2].trim());
                        SwingUtilities.invokeLater(RobotController::updateSonarUI);
                    }
                }
            } catch (SocketTimeoutException ex) {
                // 2s with no data - keep waiting, server may be busy
            } catch (NumberFormatException ex) {
                // bad sonar number - ignore and continue
            } catch (IOException ex) {
                SwingUtilities.invokeLater(() -> lblStatus.setText("Disconnesso"));
                System.out.println("Connessione persa: " + ex.getMessage());
                break;
            }
        }
    }

    static void updateSonarUI() {
        lblFront.setText(sonarHtml("FRONT", distFront));
        lblLeft .setText(sonarHtml("SX",    distLeft));
        lblRight.setText(sonarHtml("DX",    distRight));
    }

    static String sonarHtml(String name, int val) {
        String col = val < 10 ? "#ff4444" : val < 25 ? "#ffaa00" : "#00dcb4";
        return "<html><center><b><font color='#00dcb4'>" + name + "</font></b>"
             + "<br><font size='5' color='" + col + "'>" + val + "</font></center></html>";
    }
}