using System;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Net.NetworkInformation;
using System.Diagnostics;
using System.Security.AccessControl;

namespace Icinga
{
	public partial class SetupWizard : Form
	{
		private string _TrustedFile;
		private string Icinga2User;

		public SetupWizard()
		{
			InitializeComponent();

			txtInstanceName.Text = Icinga2InstanceName;

			Icinga2User = Program.Icinga2User;
			txtUser.Text = Icinga2User;
		}

		private void Warning(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
		}

		private string Icinga2InstanceName
		{
			get
			{
				IPGlobalProperties props = IPGlobalProperties.GetIPGlobalProperties();

				string fqdn = props.HostName;

				if (props.DomainName != "")
					fqdn += "." + props.DomainName;

				return fqdn.ToLower();
			}
		}

		private bool GetMasterHostPort(out string host, out string port)
		{
			foreach (ListViewItem lvi in lvwEndpoints.Items) {
				if (lvi.SubItems.Count > 1) {
					host = lvi.SubItems[1].Text.Trim();
					port = lvi.SubItems[2].Text.Trim();
					return true;
				}
			}

			host = null;
			port = null;
			return false;
		}

		private void EnableFeature(string feature)
		{
			FileStream fp = null;
			try {
				fp = File.Open(Program.Icinga2DataDir + String.Format("\\etc\\icinga2\\features-enabled\\{0}.conf", feature), FileMode.Create);
				using (StreamWriter sw = new StreamWriter(fp, Encoding.ASCII)) {
					fp = null;
					sw.Write(String.Format("include \"../features-available/{0}.conf\"\n", feature));
				}
			} finally {
				if (fp != null)
					fp.Dispose();
			}
		}

		private void SetRetrievalStatus(int pct)
		{
			if (InvokeRequired) {
				Invoke((MethodInvoker)delegate { SetRetrievalStatus(pct); });
				return;
			}

			prgRetrieveCertificate.Value = pct;
		}

		private void SetConfigureStatus(int pct, string message)
		{
			if (InvokeRequired) {
				Invoke((MethodInvoker)delegate { SetConfigureStatus(pct, message); });
				return;
			}

			prgConfig.Value = pct;
			lblConfigStatus.Text = message;
		}

		private void ShowErrorText(string text)
		{
			if (InvokeRequired) {
				Invoke((MethodInvoker)delegate { ShowErrorText(text); });
				return;
			}

			txtError.Text = text;
			tbcPages.SelectedTab = tabError;
		}

		private bool RunProcess(string filename, string arguments, out string output)
		{
			ProcessStartInfo psi = new ProcessStartInfo();
			psi.FileName = filename;
			psi.Arguments = arguments;
			psi.CreateNoWindow = true;
			psi.UseShellExecute = false;
			psi.RedirectStandardOutput = true;
			psi.RedirectStandardError = true;

			String result = "";

			using (Process proc = Process.Start(psi)) {
				proc.ErrorDataReceived += delegate (object sender, DataReceivedEventArgs args)
				{
					result += args.Data + "\r\n";
				};
				proc.OutputDataReceived += delegate (object sender, DataReceivedEventArgs args)
				{
					result += args.Data + "\r\n";
				};
				proc.BeginOutputReadLine();
				proc.BeginErrorReadLine();
				proc.WaitForExit();

				output = result;

				if (proc.ExitCode != 0)
					return false;
			}

			return true;
		}

		private void VerifyCertificate(string host, string port)
		{
			SetRetrievalStatus(25);

			string pathPrefix = Program.Icinga2DataDir + "\\etc\\icinga2\\pki\\" + txtInstanceName.Text;
			string processArguments = "pki new-cert --cn \"" + txtInstanceName.Text + "\" --key \"" + pathPrefix + ".key\" --cert \"" + pathPrefix + ".crt\"";
			string output;

			if (!File.Exists(pathPrefix + ".crt")) {
				if (!RunProcess(Program.Icinga2InstallDir + "\\sbin\\icinga2.exe",
					processArguments,
					out output)) {
					ShowErrorText("Running command 'icinga2.exe " + processArguments + "' produced the following output:\n" + output);
					return;
				}
			}

			SetRetrievalStatus(50);

			_TrustedFile = Path.GetTempFileName();

			processArguments = "pki save-cert --host \"" + host + "\" --port \"" + port + "\" --trustedcert \"" + _TrustedFile + "\"";
			if (!RunProcess(Program.Icinga2InstallDir + "\\sbin\\icinga2.exe",
				processArguments,
				out output)) {
				ShowErrorText("Running command 'icinga2.exe " + processArguments + "' produced the following output:\n" + output);
				return;
			}

			SetRetrievalStatus(100);
			try {
				X509Certificate2 cert = new X509Certificate2(_TrustedFile);
				Invoke((MethodInvoker)delegate { ShowCertificatePrompt(cert); });
			} catch (Exception e) {
				ShowErrorText("Failed to receive certificate: " + e.Message);
			}
		}

		private void ConfigureService()
		{
			SetConfigureStatus(0, "Updating configuration files...");

			string output;

			string args = "";

			Invoke((MethodInvoker)delegate
			{
				string master_host, master_port;
				GetMasterHostPort(out master_host, out master_port);

				args += " --master_host " + master_host + "," + master_port;

				foreach (ListViewItem lvi in lvwEndpoints.Items) {
					args += " --endpoint " + lvi.SubItems[0].Text.Trim();

					if (lvi.SubItems.Count > 1)
						args += "," + lvi.SubItems[1].Text.Trim() + "," + lvi.SubItems[2].Text.Trim();
				}
			});

			if (rdoListener.Checked)
				args += " --listen ::," + txtListenerPort.Text.Trim();

			if (chkAcceptConfig.Checked)
				args += " --accept-config";

			if (chkAcceptCommands.Checked)
				args += " --accept-commands";

			string ticket = txtTicket.Text.Trim();

			if (ticket.Length > 0)
				args += " --ticket \"" + ticket + "\"";

			args += " --trustedcert \"" + _TrustedFile + "\"";
			args += " --cn \"" + txtInstanceName.Text.Trim() + "\"";
			args += " --zone \"" + txtInstanceName.Text.Trim() + "\"";

			foreach (ListViewItem lvi in lvwGlobalZones.Items) {
				args += " --global_zones " + lvi.SubItems[0].Text.Trim();
			}

			if (chkDisableConf.Checked)
				args += " --disable-confd";

			if (!RunProcess(Program.Icinga2InstallDir + "\\sbin\\icinga2.exe",
				"node setup" + args,
				out output)) {
				ShowErrorText("Running command 'icinga2.exe " + "node setup" + args + "' produced the following output:\n" + output);
				return;
			}

			SetConfigureStatus(50, "Setting ACLs for the Icinga 2 directory...");

			string serviceUser = txtUser.Text.Trim();

			DirectoryInfo di = new DirectoryInfo(Program.Icinga2InstallDir);
			DirectorySecurity ds = di.GetAccessControl();
			FileSystemAccessRule rule = new FileSystemAccessRule(serviceUser,
				FileSystemRights.Modify,
				InheritanceFlags.ObjectInherit | InheritanceFlags.ContainerInherit, PropagationFlags.None, AccessControlType.Allow);
			try {
				ds.AddAccessRule(rule);
				di.SetAccessControl(ds);
			} catch (System.Security.Principal.IdentityNotMappedException) {
				ShowErrorText("Could not set ACLs for user \"" + serviceUser + "\". Identitiy is not mapped.\n");
				return;
			}

			SetConfigureStatus(75, "Installing the Icinga 2 service...");

			RunProcess(Program.Icinga2InstallDir + "\\sbin\\icinga2.exe",
				"--scm-uninstall",
				out output);

			if (!RunProcess(Program.Icinga2InstallDir + "\\sbin\\icinga2.exe",
				"daemon --validate",
				out output)) {
				ShowErrorText("Running command 'icinga2.exe daemon --validate' produced the following output:\n" + output);
				return;
			}

			if (!RunProcess(Program.Icinga2InstallDir + "\\sbin\\icinga2.exe",
				"--scm-install --scm-user \"" + serviceUser + "\" daemon",
				out output)) {
				ShowErrorText("\nRunning command 'icinga2.exe --scm-install --scm-user \"" +
					serviceUser + "\" daemon' produced the following output:\n" + output);
				return;
			}

			SetConfigureStatus(100, "Finished.");

			// Override the completed text
			lblSetupCompleted.Text = "The Icinga Windows agent was set up successfully.";

			// Add a note for the user for ticket-less signing
			if (ticket.Length == 0) {
				lblSetupCompleted.Text += "\n\nTicket was not specified. Please sign the certificate request on the Icinga 2 master node (requires v2.8+).";
			}

			FinishConfigure();
		}

		private void FinishConfigure()
		{
			if (InvokeRequired) {
				Invoke((MethodInvoker)FinishConfigure);
				return;
			}

			tbcPages.SelectedTab = tabFinish;
		}

		private void btnBack_Click(object sender, EventArgs e)
		{
			if (tbcPages.SelectedTab == tabError) {
				tbcPages.SelectedIndex = 0;
				return;
			}

			int offset = 1;

			if (tbcPages.SelectedTab == tabVerifyCertificate)
				offset++;

			tbcPages.SelectedIndex -= offset;
		}

		private void btnNext_Click(object sender, EventArgs e)
		{
			if (tbcPages.SelectedTab == tabParameters) {
				if (txtInstanceName.Text.Length == 0) {
					Warning("Please enter an instance name.");
					return;
				}

				if (lvwEndpoints.Items.Count == 0) {
					Warning("You need to add at least one master/satellite endpoint.");
					return;
				}

				string host, port;
				if (!GetMasterHostPort(out host, out port)) {
					Warning("Please enter a remote host and port for at least one of your endpoints.");
					return;
				}

				if (rdoListener.Checked && (txtListenerPort.Text == "")) {
					Warning("You need to specify a listener port.");
					return;
				}

				if (txtUser.Text.Length == 0) {
					Warning("Icinga 2 service user may not be empty.");
					return;
				}
			}

			if (tbcPages.SelectedTab == tabFinish || tbcPages.SelectedTab == tabError)
				Application.Exit();

			tbcPages.SelectedIndex++;
		}

		private void btnCancel_Click(object sender, EventArgs e)
		{
			Application.Exit();
		}

		private void tbcPages_SelectedIndexChanged(object sender, EventArgs e)
		{
			Refresh();

			btnBack.Enabled = (tbcPages.SelectedTab == tabVerifyCertificate || tbcPages.SelectedTab == tabError);
			btnNext.Enabled = (tbcPages.SelectedTab == tabParameters || tbcPages.SelectedTab == tabVerifyCertificate || tbcPages.SelectedTab == tabFinish);

			if (tbcPages.SelectedTab == tabFinish) {
				btnNext.Text = "&Finish >";
				btnCancel.Enabled = false;
			}

			if (tbcPages.SelectedTab == tabRetrieveCertificate) {
				ListViewItem lvi = lvwEndpoints.Items[0];

				string master_host, master_port;
				GetMasterHostPort(out master_host, out master_port);

				Thread thread = new Thread((ThreadStart)delegate { VerifyCertificate(master_host, master_port); });
				thread.Start();
			}

			if (tbcPages.SelectedTab == tabConfigure) {
				Thread thread = new Thread(ConfigureService);
				thread.Start();
			}
		}

		private void RadioListener_CheckedChanged(object sender, EventArgs e)
		{
			txtListenerPort.Enabled = rdoListener.Checked;
		}

		private void AddCertificateField(string name, string shortValue, string longValue = null)
		{
			ListViewItem lvi = new ListViewItem();
			lvi.Text = name;
			lvi.SubItems.Add(shortValue);
			if (longValue == null)
				longValue = shortValue;
			lvi.Tag = longValue;
			lvwX509Fields.Items.Add(lvi);
		}

		private string PadText(string input)
		{
			string output = "";

			for (int i = 0; i < input.Length; i += 2) {
				if (output != "")
					output += " ";

				int len = 2;
				if (input.Length - i < 2)
					len = input.Length - i;
				output += input.Substring(i, len);
			}

			return output;
		}

		private void ShowCertificatePrompt(X509Certificate2 certificate)
		{
			txtX509Issuer.Text = certificate.Issuer;
			txtX509Subject.Text = certificate.Subject;

			lvwX509Fields.Items.Clear();

			AddCertificateField("Version", "V" + certificate.Version.ToString());
			AddCertificateField("Serial number", certificate.SerialNumber);
			AddCertificateField("Signature algorithm", certificate.SignatureAlgorithm.FriendlyName);
			AddCertificateField("Valid from", certificate.NotBefore.ToString());
			AddCertificateField("Valid to", certificate.NotAfter.ToString());

			string pkey = BitConverter.ToString(certificate.PublicKey.EncodedKeyValue.RawData).Replace("-", " ");
			AddCertificateField("Public key", certificate.PublicKey.Oid.FriendlyName + " (" + certificate.PublicKey.Key.KeySize + " bits)", pkey);

			string thumbprint = PadText(certificate.Thumbprint);
			AddCertificateField("Thumbprint", thumbprint);

			tbcPages.SelectedTab = tabVerifyCertificate;
		}

		private void btnAddEndpoint_Click(object sender, EventArgs e)
		{
			EndpointInputBox eib = new EndpointInputBox();

			if (eib.ShowDialog(this) == DialogResult.Cancel)
				return;

			ListViewItem lvi = new ListViewItem();
			lvi.Text = eib.txtInstanceName.Text;

			if (eib.chkConnect.Checked) {
				lvi.SubItems.Add(eib.txtHost.Text);
				lvi.SubItems.Add(eib.txtPort.Text);
			}

			lvwEndpoints.Items.Add(lvi);
		}

		private void lvwEndpoints_SelectedIndexChanged(object sender, EventArgs e)
		{
			btnRemoveEndpoint.Enabled = lvwEndpoints.SelectedItems.Count > 0;
			btnEditEndpoint.Enabled = lvwEndpoints.SelectedItems.Count > 0;
		}

		private void lvwX509Fields_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (lvwX509Fields.SelectedItems.Count == 0)
				return;

			ListViewItem lvi = lvwX509Fields.SelectedItems[0];

			txtX509Field.Text = Convert.ToString(lvi.Tag);
		}

		private void btnRemoveEndpoint_Click(object sender, EventArgs e)
		{
			while (lvwEndpoints.SelectedItems.Count > 0) {
				lvwEndpoints.Items.Remove(lvwEndpoints.SelectedItems[0]);
			}
		}

		private void chkRunServiceAsThisUser_CheckedChanged(object sender, EventArgs e)
		{
			txtUser.Enabled = !txtUser.Enabled;
			if (!txtUser.Enabled)
				txtUser.Text = Icinga2User;
		}

		private void btnEditEndpoint_Click(object sender, EventArgs e)
		{
			ListViewItem lvi = lvwEndpoints.SelectedItems[0];
			EndpointInputBox eib = new EndpointInputBox();

			eib.Text = "Edit Endpoint";
			eib.txtInstanceName.Text = lvi.SubItems[0].Text;

			if (lvi.SubItems.Count >= 2) {
				eib.txtHost.Text = lvi.SubItems[1].Text;
				eib.txtPort.Text = lvi.SubItems[2].Text;
				eib.chkConnect.Checked = true;
			}

			if (eib.ShowDialog(this) == DialogResult.Cancel)
				return;

			lvwEndpoints.Items.Remove(lvi);

			ListViewItem lvi2 = new ListViewItem();
			lvi2.Text = eib.txtInstanceName.Text;

			if (eib.chkConnect.Checked) {
				lvi2.SubItems.Add(eib.txtHost.Text);
				lvi2.SubItems.Add(eib.txtPort.Text);
			}

			lvwEndpoints.Items.Add(lvi2);
		}

		private void btnAddGlobalZone_Click(object sender, EventArgs e)
		{
			GlobalZonesInputBox gzib = new GlobalZonesInputBox(lvwGlobalZones.Items);

			if (gzib.ShowDialog(this) == DialogResult.Cancel)
				return;

			ListViewItem lvi = new ListViewItem();
			lvi.Text = gzib.txtGlobalZoneName.Text;

			lvwGlobalZones.Items.Add(lvi);
		}

		private void btnRemoveGlobalZone_Click(object sender, EventArgs e)
		{
			while (lvwGlobalZones.SelectedItems.Count > 0) {
				lvwGlobalZones.Items.Remove(lvwGlobalZones.SelectedItems[0]);
			}
		}

		private void lvwGlobalZones_SelectedIndexChanged(object sender, EventArgs e)
		{
			btnEditGlobalZone.Enabled = lvwGlobalZones.SelectedItems.Count > 0;
			btnRemoveGlobalZone.Enabled = lvwGlobalZones.SelectedItems.Count > 0;
		}

		private void btnEditGlobalZone_Click(object sender, EventArgs e)
		{
			ListViewItem lvi = lvwGlobalZones.SelectedItems[0];
			GlobalZonesInputBox gzib = new GlobalZonesInputBox(lvwGlobalZones.Items);

			gzib.Text = "Edit Global Zone";
			gzib.txtGlobalZoneName.Text = lvi.SubItems[0].Text;
			
			if (gzib.ShowDialog(this) == DialogResult.Cancel)
				return;

			lvwGlobalZones.Items.Remove(lvi);

			ListViewItem lvi2 = new ListViewItem();
			lvi2.Text = gzib.txtGlobalZoneName.Text;
			
			lvwGlobalZones.Items.Add(lvi2);
		}

		private void SetupWizard_Load(object sender, EventArgs e)
		{
			this.MinimumSize = this.Size;
			this.MaximumSize = this.Size;
		}

		private void linkLabelDocs_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
		{
			linkLabelDocs.LinkVisited = true;

			Process.Start("https://icinga.com/docs/icinga2/latest/");
		}
	}
}

