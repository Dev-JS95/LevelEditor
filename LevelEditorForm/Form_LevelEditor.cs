using LevelEditorWrap;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace LevelEditorForm
{
    public partial class LevelEditorForm : Form
    {
        //자식 폼
        ViewportForm mViewportForm;
        AssetBrowserForm mAssetBrowserForm;

        //게임 루프 스레드
        Thread mGameLoopThread;
        bool bCanGameLoop;

        //LevelEditor
        LevelEditorWrapper editor;

        public LevelEditorForm()
        {
            InitializeComponent();

            //MDI 자식들 추가
            mAssetBrowserForm = new AssetBrowserForm();
            mAssetBrowserForm.MdiParent = this;
            mAssetBrowserForm.StartPosition = FormStartPosition.Manual;
            mAssetBrowserForm.Location = new Point(10, 550);
            mAssetBrowserForm.Show();

            mViewportForm = new ViewportForm();
            mViewportForm.MdiParent = this;
            mViewportForm.StartPosition = FormStartPosition.Manual;
            mViewportForm.Location = new Point(10, 10);
            mViewportForm.Show();



            try
            {
                //에디터 생성
                editor = new LevelEditorWrapper(mViewportForm.P_Viewport.Handle);

                //에셋브라우저 초기화
                var fileList = editor.GetFileList();
                foreach (var elem in fileList)
                {
                    if (elem.FileType == EFileType.Directory)
                        mAssetBrowserForm.LV_Browser.Items.Add(elem.FileName, 0);
                    else
                        mAssetBrowserForm.LV_Browser.Items.Add(elem.FileName, 1);
                }
           
                //게임 루프 연결
                bCanGameLoop = true;
                mGameLoopThread = new Thread(GameLoop);
                mGameLoopThread.IsBackground = true;
                mGameLoopThread.Start();

            }
            catch (Exception exception)
            {
                MessageBox.Show(exception.ToString());

                //정리
                Close();
            }
        }

        //게임 루프
        void GameLoop()
        {
            while (true)
            {
                if (bCanGameLoop)
                {
                    editor.Tick();
                }
                else
                {
                    System.Threading.Thread.Sleep(300);
                }
            }
        }
        private void LevelEditorForm_Activated(object sender, EventArgs e)
        {
            bCanGameLoop = true;
        }

        private void LevelEditorForm_Deactivate(object sender, EventArgs e)
        {
            bCanGameLoop = false;
        }

    }
}
