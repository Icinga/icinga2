using System;
using System.IO;
using System.Text;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Net.NetworkInformation;
using Microsoft.Win32;
using System.IO.Compression;
using System.Diagnostics;
using System.ServiceProcess;
using System.Security.AccessControl;

namespace Icinga
{
	public partial class AgentWizard : Form
	{
		public AgentWizard()
		{
			InitializeComponent();

			txtInstanceName.Text = Icinga2InstanceName;
		}

		private void FatalError(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Error);
			Application.Exit();
		}

		private void Warning(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
		}

		private string Icinga2InstallDir
		{
			get
			{
				RegistryKey rk = Registry.LocalMachine.OpenSubKey("SOFTWARE\\Icinga Development Team\\ICINGA2");

				if (rk == null)
					return "";

				return (string)rk.GetValue("");
			}
		}

		private string Icinga2InstanceName
		{
			get
			{
				IPGlobalProperties props = IPGlobalProperties.GetIPGlobalProperties();

				string fqdn = props.HostName;

				if (props.DomainName != "")
					fqdn += "." + props.DomainName;

				return fqdn;
			}
		}

		private void EnableFeature(string feature)
		{
			using (FileStream fp = File.Open(Icinga2InstallDir + String.Format("\\etc\\icinga2\\features-enabled\\{0}.conf", feature), FileMode.Create)) {
				using (StreamWriter sw = new StreamWriter(fp, Encoding.ASCII)) {
					sw.Write(String.Format("include \"../features-available/{0}.conf\"\n", feature));
				}
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

		private void VerifyCertificate(string host, string port)
		{
			SetRetrievalStatus(25);

			string pathPrefix = Icinga2InstallDir + "\\etc\\icinga2\\pki\\" + txtInstanceName.Text;

			ProcessStartInfo psi;

			if (!File.Exists(pathPrefix + ".crt")) {
				psi = new ProcessStartInfo();
				psi.FileName = Icinga2InstallDir + "\\sbin\\icinga2.exe";
				psi.Arguments = "pki new-cert --cn \"" + txtInstanceName.Text + "\" --keyfile \"" + pathPrefix + ".key\" --certfile \"" + pathPrefix + ".crt\"";
				psi.CreateNoWindow = true;
				psi.UseShellExecute = false;

				using (Process proc = Process.Start(psi)) {
					proc.WaitForExit();

					if (proc.ExitCode != 0) {
						Invoke((MethodInvoker)delegate { FatalError("The Windows service could not be installed."); });
						return;
					}
				}
			}

			SetRetrievalStatus(50);

			string trustedfile = Path.GetTempFileName();

			psi = new ProcessStartInfo();
			psi.FileName = Icinga2InstallDir + "\\sbin\\icinga2.exe";
			psi.Arguments = "pki save-cert --host \"" + host + "\" --port \"" + port + "\" --keyfile \"" + pathPrefix + ".key\" --certfile \"" + pathPrefix + ".crt\" --trustedfile \"" + trustedfile + "\"";
			psi.CreateNoWindow = true;
			psi.UseShellExecute = false;

			using (Process proc = Process.Start(psi)) {
				proc.WaitForExit();

				if (proc.ExitCode != 0) {
					Invoke((MethodInvoker)delegate { FatalError("Could not retrieve the master's X509 certificate."); });
					return;
				}
			}

			SetRetrievalStatus(100);
	
			X509Certificate2 cert = new X509Certificate2(trustedfile);
			Invoke((MethodInvoker)delegate { ShowCertificatePrompt(cert); });
		}

		private void ConfigureService()
		{
			SetConfigureStatus(0, "Updating configuration files...");
			using (FileStream fp = File.Open(Icinga2InstallDir + "\\etc\\icinga2\\features-available\\agent.conf", FileMode.Create)) {
				using (StreamWriter sw = new StreamWriter(fp, Encoding.ASCII)) {
					sw.Write(
					    "/**\n" +
					    " * The agent listener accepts checks from agents.\n" +
					    " */\n" +
					    "\n" +
					    "library \"agent\"\n" +
					    "\n" +
					    "object AgentListener \"agent\" {\n" +
					    "  cert_path = SysconfDir + \"/icinga2/pki/agent/agent.crt\"\n" +
					    "  key_path = SysconfDir + \"/icinga2/pki/agent/agent.key\"\n" +
					    "  ca_path = SysconfDir + \"/icinga2/pki/agent/ca.crt\"\n"
					);

					/*if (rdoNoMaster.Checked)
						sw.Write("  upstream_name = \"{0}\"\n", txtMasterInstance.Text);*/

					if (rdoListener.Checked)
						sw.Write("  bind_port = \"{0}\"\n", txtListenerPort.Text);

					/*if (rdoConnect.Checked)
						sw.Write(
						    "  upstream_host = \"{0}\"\n" +
						    "  upstream_port = \"{1}\"\n", txtPeerHost.Text, txtPeerPort.Text
						);*/

					sw.Write("}\n");
				}
			}

			EnableFeature("api");
			EnableFeature("checker");

			SetConfigureStatus(50, "Setting ACLs for the Icinga 2 directory...");
			DirectoryInfo di = new DirectoryInfo(Icinga2InstallDir);
			DirectorySecurity ds = di.GetAccessControl();
			FileSystemAccessRule rule = new FileSystemAccessRule("NT AUTHORITY\\NetworkService",
			    FileSystemRights.ReadAndExecute | FileSystemRights.Write | FileSystemRights.ListDirectory,
			    InheritanceFlags.ObjectInherit | InheritanceFlags.ContainerInherit, PropagationFlags.None, AccessControlType.Allow);
			ds.AddAccessRule(rule);
			di.SetAccessControl(ds);

			SetConfigureStatus(75, "Installing the Icinga 2 service...");

			ProcessStartInfo psi = new ProcessStartInfo();
			psi.FileName = Icinga2InstallDir + "\\sbin\\icinga2.exe";
			psi.Arguments = "--scm-uninstall";
			psi.CreateNoWindow = true;
			psi.UseShellExecute = false;

			using (Process proc = Process.Start(psi)) {
				proc.WaitForExit();
			}
			
			psi = new ProcessStartInfo();
			psi.FileName = Icinga2InstallDir + "\\sbin\\icinga2.exe";
			psi.Arguments = "--scm-install daemon";
			psi.CreateNoWindow = true;
			psi.UseShellExecute = false;

			using (Process proc = Process.Start(psi)) {
				proc.WaitForExit();

				if (proc.ExitCode != 0) {
					Invoke((MethodInvoker)delegate { FatalError("The Windows service could not be installed."); });
					return;
				}
			}

			SetConfigureStatus(100, "Finished.");

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

		private void AgentWizard_Shown(object sender, EventArgs e)
		{
			string installDir = Icinga2InstallDir;

			if (installDir == "")
				FatalError("Icinga 2 does not seem to be installed properly.");

			/* TODO: This is something the NSIS installer should do */
			Directory.CreateDirectory(installDir + "\\var\\cache\\icinga2");
			Directory.CreateDirectory(installDir + "\\var\\lib\\icinga2\\agent\\inventory");
			Directory.CreateDirectory(installDir + "\\var\\lib\\icinga2\\cluster\\config");
			Directory.CreateDirectory(installDir + "\\var\\lib\\icinga2\\cluster\\log");
			Directory.CreateDirectory(installDir + "\\var\\log\\icinga2\\compat\\archive");
			Directory.CreateDirectory(installDir + "\\var\\run\\icinga2\\cmd");
			Directory.CreateDirectory(installDir + "\\var\\spool\\icinga2\\perfdata");
			Directory.CreateDirectory(installDir + "\\var\\spool\\icinga2\\tmp");

			Directory.CreateDirectory(installDir + "\\etc\\icinga2\\pki\\agent");

			
		}

		private void btnBack_Click(object sender, EventArgs e)
		{
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

				if (rdoNoMaster.Checked && lvwEndpoints.Items.Count == 0) {
					Warning("You need to add at least one master endpoint.");
					return;
				}

				if (rdoListener.Checked && (txtListenerPort.Text == "")) {
					Warning("You need to specify a listener port.");
					return;
				}
			}

			if (tbcPages.SelectedTab == tabFinish)
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

			btnBack.Enabled = (tbcPages.SelectedTab == tabVerifyCertificate);
			btnNext.Enabled = (tbcPages.SelectedTab == tabParameters || tbcPages.SelectedTab == tabVerifyCertificate || tbcPages.SelectedTab == tabFinish);

			if (tbcPages.SelectedTab == tabFinish) {
				btnNext.Text = "&Finish >";
				btnCancel.Enabled = false;
			}

			if (tbcPages.SelectedTab == tabRetrieveCertificate) {
				ListViewItem lvi = lvwEndpoints.Items[0];

				Thread thread = new Thread((ThreadStart)delegate { VerifyCertificate(lvi.SubItems[0].Text, lvi.SubItems[1].Text); });
				thread.Start();
			}

			/*if (tbcPages.SelectedTab == tabParameters &&
			    !File.Exists(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent\\agent.crt")) {
				byte[] bytes = Convert.FromBase64String(txtBundle.Text);
				MemoryStream ms = new MemoryStream(bytes);
				GZipStream gz = new GZipStream(ms, CompressionMode.Decompress);
				MemoryStream ms2 = new MemoryStream();

				byte[] buffer = new byte[512];
				int rc;
				while ((rc = gz.Read(buffer, 0, buffer.Length)) > 0)
					ms2.Write(buffer, 0, rc);
				ms2.Position = 0;
				TarReader tr = new TarReader(ms2);
				tr.ReadToEnd(Icinga2InstallDir + "\\etc\\icinga2\\pki\\agent");
			}*/

			if (tbcPages.SelectedTab == tabConfigure) {
				Thread thread = new Thread(ConfigureService);
				thread.Start();
			}
		}

		private void RadioMaster_CheckedChanged(object sender, EventArgs e)
		{
			lvwEndpoints.Enabled = !rdoNewMaster.Checked;
			btnAddEndpoint.Enabled = !rdoNewMaster.Checked;
			btnRemoveEndpoint.Enabled = !rdoNewMaster.Checked && lvwEndpoints.SelectedItems.Count > 0;
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
			lvi.Text = eib.txtHost.Text;
			lvi.SubItems.Add(eib.txtPort.Text);

			lvwEndpoints.Items.Add(lvi);
		}

		private void lvwEndpoints_SelectedIndexChanged(object sender, EventArgs e)
		{
			btnRemoveEndpoint.Enabled = lvwEndpoints.SelectedItems.Count > 0;
		}

		private void lvwX509Fields_SelectedIndexChanged(object sender, EventArgs e)
		{
			if (lvwX509Fields.SelectedItems.Count == 0)
				return;

			ListViewItem lvi = lvwX509Fields.SelectedItems[0];

			txtX509Field.Text = (string)lvi.Tag;
		}

		private void btnRemoveEndpoint_Click(object sender, EventArgs e)
		{
			while (lvwEndpoints.SelectedItems.Count > 0) {
				lvwEndpoints.Items.Remove(lvwEndpoints.SelectedItems[0]);
			}
		}
	}
}
