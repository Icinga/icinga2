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

		private void txtHost_Validating(object sender, CancelEventArgs e)
		{
			if (txtHost.Text.Length == 0) {
				e.Cancel = true;
				errErrorProvider.SetError(txtHost, "Please enter a host name.");
			}
		}

		private void txtPort_Validating(object sender, CancelEventArgs e)
		{
			if (txtPort.Text.Length == 0) {
				e.Cancel = true;
				errErrorProvider.SetError(txtPort, "Please enter a port.");
			}
		}
	}
}
