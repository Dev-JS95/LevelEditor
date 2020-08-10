namespace LevelEditorForm
{
    partial class ViewportForm
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
            this.SuspendLayout();
            // 
            // P_Viewport
            // 
            this.P_Viewport.Dock = System.Windows.Forms.DockStyle.Fill;
            this.P_Viewport.Location = new System.Drawing.Point(0, 0);
            this.P_Viewport.Name = "P_Viewport";
            this.P_Viewport.Size = new System.Drawing.Size(800, 450);
            this.P_Viewport.TabIndex = 0;
            // 
            // ViewportForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(800, 450);
            this.Controls.Add(this.P_Viewport);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.SizableToolWindow;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ViewportForm";
            this.Text = "Viewport";
            this.ResumeLayout(false);

        }

        #endregion

        internal System.Windows.Forms.Panel P_Viewport;
    }
}