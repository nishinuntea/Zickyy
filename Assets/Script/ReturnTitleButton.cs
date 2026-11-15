/*==============================================================================

    タイトル戻るボタン		 [ReturnTitleButton.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.SceneManagement;

public class ReturnTitleButton : MonoBehaviour
{
    [Header("移動先のタイトルシーン名")]
    [SerializeField] private string titleSceneName = "Title"; // タイトルシーン名に変更してください

    // ボタンが押されたときに呼ぶ関数
    public void OnClick()
    {
        Time.timeScale = 1f; // 念のため時間を通常に戻す
        SceneManager.LoadScene(titleSceneName);
    }
}
