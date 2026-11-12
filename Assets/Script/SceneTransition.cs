using UnityEngine;
using UnityEngine.SceneManagement;
using UnityEngine.InputSystem;

#if UNITY_EDITOR
using UnityEditor;
#endif

//====================================================
//シーン遷移の管理
//更新日　2026/11/12
//國友
//====================================================
public class SceneTransition : MonoBehaviour
{
#if UNITY_EDITOR
    //inspectorで遷移先のSceneをドラックして設定する
    [SerializeField] private SceneAsset NextScene;


    //inspectorで設定を変更したときなどに自動で呼ばれる
    private void OnValidate()
    {
        //Sceneが設定されていればそのファイルパスを取得する
        scenePath = NextScene != null ? AssetDatabase.GetAssetPath(NextScene) : "";
    }
#endif

    //実行時に使うシーンのパスを保存する
    [SerializeField, HideInInspector]
    private string scenePath;

    //ロード中か記録して、連打による二重実行を防ぐ
    private bool isLoading;
    private void Update()
    {
        // キーボードが接続されていなければ何もしない
        if (Keyboard.current == null) return;

        // Enter、またはテンキーのEnterを押した瞬間に遷移する
        if (Keyboard.current.enterKey.wasPressedThisFrame ||
            Keyboard.current.numpadEnterKey.wasPressedThisFrame)
        {
            Transition();
        }
    }    //別スクリプトから呼びだす
    public void Transition()
    {
        //すでにロード中なら何もしない
        if (isLoading)
        {
            return;
        }

        //Sceneが未設定、またはロードできない場合は処理を止める
        if (string.IsNullOrEmpty(scenePath) || !Application.CanStreamedLevelBeLoaded(scenePath))
        {
            Debug.LogError("遷移先のSceneがないっす", this);
            return;
        }
        //ロード開始
        isLoading = true;

        //遷移先のSceneを非同期で読み込む
        //読み込みが完了すると
        SceneManager.LoadSceneAsync(scenePath);

    }
}
