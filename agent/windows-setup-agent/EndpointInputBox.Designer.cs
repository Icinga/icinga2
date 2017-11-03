namespace Icinga
{
	partial class EndpointInputBox
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null)) {
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btnOK = new System.Windows.Forms.Button();
			this.btnCancel = new System.Windows.Forms.Button();
			this.txtHost = new System.Windows.Forms.TextBox();
			this.txtPort = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.lblHost = new System.Windows.Forms.Label();
			this.lblPort = new System.Windows.Forms.Label();
			this.lblInstanceName = new System.Windows.Forms.Label();
			this.txtInstanceName = new System.Windows.Forms.TextBox();
			this.chkConnect = new System.Windows.Forms.CheckBox();
			this.SuspendLayout();
			// 
			// btnOK
			// 
			this.btnOK.Location = new System.Drawing.Point(196, 171);
			this.btnOK.Name = "btnOK";
			this.btnOK.Size = new System.Drawing.Size(75, 23);
			this.btnOK.TabIndex = 4;
			this.btnOK.Text = "OK";
			this.btnOK.UseVisualStyleBackColor = true;
			this.btnOK.Click += new System.EventHandler(this.btnOK_Click);
			// 
			// btnCancel
			// 
			this.btnCancel.CausesValidation = false;
			this.btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.btnCancel.Location = new System.Drawing.Point(277, 171);
			this.btnCancel.Name = "btnCancel";
			this.btnCancel.Size = new System.Drawing.Size(75, 23);
			this.btnCancel.TabIndex = 5;
			this.btnCancel.Text = "Cancel";
			this.btnCancel.UseVisualStyleBackColor = true;
			// 
			// txtHost
			// 
			this.txtHost.Location = new System.Drawing.Point(101, 103);
			this.txtHost.Name = "txtHost";
			this.txtHost.Size = new System.Drawing.Size(251, 20);
			this.txtHost.TabIndex = 2;
			// 
			// txtPort
			// 
			this.txtPort.Location = new System.Drawing.Point(101, 134);
			this.txtPort.Name = "txtPort";
			this.txtPort.Size = new System.Drawing.Size(100, 20);
			this.txtPort.TabIndex = 3;
			this.txtPort.Text = "5665";
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(12, 9);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(276, 13);
			this.label1.TabIndex = 4;
			this.label1.Text = "Please enter the connection details for the new endpoint:";
			// 
			// lblHost
			// 
			this.lblHost.AutoSize = true;
			this.lblHost.Location = new System.Drawing.Point(15, 106);
			this.lblHost.Name = "lblHost";
			this.lblHost.Size = new System.Drawing.Size(32, 13);
			this.lblHost.TabIndex = 5;
			this.lblHost.Text = "Host:";
			// 
			// lblPort
			// 
			this.lblPort.AutoSize = true;
			this.lblPort.Location = new System.Drawing.Point(15, 137);
			this.lblPort.Name = "lblPort";
			this.lblPort.Size = new System.Drawing.Size(29, 13);
			this.lblPort.TabIndex = 6;
			this.lblPort.Text = "Port:";
			// 
			// lblInstanceName
			// 
			this.lblInstanceName.AutoSize = true;
			this.lblInstanceName.Location = new System.Drawing.Point(15, 41);
			this.lblInstanceName.Name = "lblInstanceName";
			this.lblInstanceName.Size = new System.Drawing.Size(82, 13);
			this.lblInstanceName.TabIndex = 7;
			this.lblInstanceName.Text = "Instance Name:";
			// 
			// txtInstanceName
			// 
			this.txtInstanceName.Location = new System.Drawing.Point(101, 37);
			this.txtInstanceName.Name = "txtInstanceName";
			this.txtInstanceName.Size = new System.Drawing.Size(251, 20);
			this.txtInstanceName.TabIndex = 0;
			// 
			// chkConnect
			// 
			this.chkConnect.AutoSize = true;
			this.chkConnect.Checked = true;
			this.chkConnect.CheckState = System.Windows.Forms.CheckState.Checked;
			this.chkConnect.Location = new System.Drawing.Point(18, 73);
			this.chkConnect.Name = "chkConnect";
			this.chkConnect.Size = new System.Drawing.Size(141, 17);
			this.chkConnect.TabIndex = 1;
			this.chkConnect.Text = "Connect to this endpoint";
			this.chkConnect.UseVisualStyleBackColor = true;
			this.chkConnect.CheckedChanged += new System.EventHandler(this.chkConnect_CheckedChanged);
			// 
			// EndpointInputBox
			// 
			this.AcceptButton = this.btnOK;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.CancelButton = this.btnCancel;
			this.ClientSize = new System.Drawing.Size(360, 202);
			this.Controls.Add(this.chkConnect);
			this.Controls.Add(this.txtInstanceName);
			this.Controls.Add(this.lblInstanceName);
			this.Controls.Add(this.lblPort);
			this.Controls.Add(this.lblHost);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.txtPort);
			this.Controls.Add(this.txtHost);
			this.Controls.Add(this.btnCancel);
			this.Controls.Add(this.btnOK);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "EndpointInputBox";
			this.ShowIcon = false;
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Add Endpoint";
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Button btnOK;
		private System.Windows.Forms.Button btnCancel;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label lblHost;
		private System.Windows.Forms.Label lblPort;
		public System.Windows.Forms.TextBox txtHost;
		public System.Windows.Forms.TextBox txtPort;
		public System.Windows.Forms.TextBox txtInstanceName;
		private System.Windows.Forms.Label lblInstanceName;
		public System.Windows.Forms.CheckBox chkConnect;
	}
}