/*==============================================================================

    ゲーム終了ボタン		 [QuitGameButton.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;

public class QuitGameButton : MonoBehaviour
{
    // ボタンが押されたときに呼ぶ関数
    public void OnClick()
    {
#if UNITY_EDITOR
        UnityEditor.EditorApplication.isPlaying = false; // エディターでのプレイを停止
#else
        Application.Quit(); // ビルドしたゲームを終了
#endif
    }
}
