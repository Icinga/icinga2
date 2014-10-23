using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace Icinga
{
	public partial class EndpointInputBox : Form
	{
		public EndpointInputBox()
		{
			InitializeComponent();
		}

		private void Warning(string message)
		{
			MessageBox.Show(this, message, Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
		}
		
		private void chkConnect_CheckedChanged(object sender, EventArgs e)
		{
			txtHost.Enabled = chkConnect.Checked;
			txtPort.Enabled = chkConnect.Checked;
		}

		private void btnOK_Click(object sender, EventArgs e)
		{
			if (txtInstanceName.Text.Length == 0) {
				Warning("Please enter an instance name.");
				return;
			}

			if (chkConnect.Checked) {
				if (txtHost.Text.Length == 0) {
					Warning("Please enter a host name.");
					return;
				}

				if (txtPort.Text.Length == 0) {
					Warning("Please enter a port.");
					return;
				}
			}

			DialogResult = DialogResult.OK;
			Close();
		}
	}
}
