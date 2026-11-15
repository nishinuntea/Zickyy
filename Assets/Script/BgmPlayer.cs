/*==============================================================================

    BGM		 [BgmPlayer.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.SceneManagement;

#if UNITY_EDITOR
using UnityEditor;
#endif

[RequireComponent(typeof(AudioSource))]
public class BgmPlayer : MonoBehaviour
{
    public static BgmPlayer Instance { get; private set; }

    [System.Serializable]
    public class BgmData
    {
#if UNITY_EDITOR
        [Header("シーンのファイル (.unity)")]
        public SceneAsset sceneAsset;
#endif
        [HideInInspector] public string sceneName;

        [Header("流すBGM")]
        public AudioClip clip;

        [Header("この曲の音量 (0.0 〜 1.0)")]
        [Range(0f, 1f)] public float volume = 0.5f;
    }

    [Header("シーンごとのBGMと音量設定")]
    [SerializeField] private BgmData[] bgmList;

    private AudioSource audioSource;
    private BgmData currentBgmData;

    private void Awake()
    {
        if (Instance != null)
        {
            Destroy(gameObject);
            return;
        }

        Instance = this;
        DontDestroyOnLoad(gameObject);

        audioSource = GetComponent<AudioSource>();
        audioSource.loop = true;
    }

#if UNITY_EDITOR
    private void OnValidate()
    {
        if (bgmList == null) return;
        foreach (var data in bgmList)
        {
            if (data.sceneAsset != null)
            {
                data.sceneName = data.sceneAsset.name;
            }
        }

        // インスペクター上でスライダーをいじったときに、プレイ中なら即座に音量を反映させる
        if (Application.isPlaying && audioSource != null && currentBgmData != null)
        {
            audioSource.volume = currentBgmData.volume;
        }
    }
#endif

    private void OnEnable()
    {
        SceneManager.sceneLoaded += OnSceneLoaded;
    }

    private void OnDisable()
    {
        SceneManager.sceneLoaded -= OnSceneLoaded;
    }

    private void OnSceneLoaded(Scene scene, LoadSceneMode mode)
    {
        currentBgmData = null;
        foreach (var data in bgmList)
        {
            if (data.sceneName == scene.name)
            {
                currentBgmData = data;
                break;
            }
        }

        if (currentBgmData != null && currentBgmData.clip != null)
        {
            if (audioSource.clip != currentBgmData.clip)
            {
                audioSource.clip = currentBgmData.clip;
            }
            audioSource.volume = currentBgmData.volume;

            if (!audioSource.isPlaying)
            {
                audioSource.Play();
            }
        }
        else
        {
            audioSource.Stop();
            audioSource.clip = null;
        }
    }
}