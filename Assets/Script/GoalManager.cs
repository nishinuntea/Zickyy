/*==============================================================================

   ゴール		 [GoalManager.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.SceneManagement;

#if UNITY_EDITOR
using UnityEditor;
#endif


public class GoalManager : MonoBehaviour
{
#if UNITY_EDITOR
    // Inspectorで遷移先のクリア画面のSceneをドラッグして設定する
    [SerializeField] private SceneAsset NextScene;

    private void OnValidate()
    {
        scenePath = NextScene != null ? AssetDatabase.GetAssetPath(NextScene) : "";
    }
#endif

    [SerializeField, HideInInspector]
    private string scenePath;

    private bool isLoading;

    // 3Dゲーム用の接触判定（ゴールオブジェクトに触れた瞬間）
    private void OnTriggerEnter(Collider other)
    {
        // ぶつかってきた相手が「Player」タグを持っている場合
        if (other.CompareTag("Player"))
        {
            Transition();
        }
    }

    // 画面遷移を実行する関数
    public void Transition()
    {
        if (isLoading)
        {
            return;
        }

        if (string.IsNullOrEmpty(scenePath) || !Application.CanStreamedLevelBeLoaded(scenePath))
        {
            Debug.LogError("遷移先のSceneが設定されていないか、ロードできません", this);
            return;
        }

        isLoading = true;
        SceneManager.LoadSceneAsync(scenePath);
    }
}
