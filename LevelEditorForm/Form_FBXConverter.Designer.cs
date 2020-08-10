namespace LevelEditorForm
{
    partial class FBXConverterForm
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
            if (disposing && (components != null))
            {
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
            this.P_Viewport = new System.Windows.Forms.Panel();
            this.BTN_Convert = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // P_Viewport
            // 
            this.P_Viewport.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.P_Viewport.Location = new System.Drawing.Point(26, 22);
            this.P_Viewport.Name = "P_Viewport";
            this.P_Viewport.Size = new System.Drawing.Size(440, 387);
            this.P_Viewport.TabIndex = 0;
            // 
            // BTN_Convert
            // 
            this.BTN_Convert.Location = new System.Drawing.Point(543, 315);
            this.BTN_Convert.Name = "BTN_Convert";
            this.BTN_Convert.Size = new System.Drawing.Size(195, 83);
            this.BTN_Convert.TabIndex = 1;
            this.BTN_Convert.Text = "Convert";
            this.BTN_Convert.UseVisualStyleBackColor = true;
            // 
            // FBXConverterForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.BTN_Convert);
            this.Controls.Add(this.P_Viewport);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "FBXConverterForm";
            this.Text = "FBXConverter";
            this.Activated += new System.EventHandler(this.FBXConverterForm_Activated);
            this.Deactivate += new System.EventHandler(this.FBXConverterForm_Deactivate);
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Panel P_Viewport;
        private System.Windows.Forms.Button BTN_Convert;
    }
}