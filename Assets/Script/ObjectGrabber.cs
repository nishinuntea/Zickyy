/*==============================================================================

    モノをつかむ		 [ObjectGrabber.cs]
														 Author : Nishimura
														 Date   : 2026/09/29
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;
using UnityEngine.InputSystem;

public class ObjectGrabber : MonoBehaviour
{
    [Header("カメラの設定")]
    [Tooltip("レイ（照準）を飛ばすためのカメラのTransformをアサインしてください")]
    public Transform cameraTransform;

    [Header("保持位置（プレイヤーからの相対位置：X=横, Y=頭上の高さ, Z=前後の距離）")]
    public Vector3 holdLocalPosition = new Vector3(0f, 2.2f, 0f); // 頭上の位置

    [Header("掴めるオブジェクトとの距離")]
    [Range(1f, 50f)]
    public float maxGrabDistance = 10f;

    [Header("掴めるオブジェクトのタグ")]
    public string grabbableTag = "MovalObject";

    [Header("反発（弾き飛ばし）設定")]
    [Tooltip("離す時にオブジェクトを弾き飛ばす強さ")]
    public float repelForce = 15f;

    // 内部処理用の変数
    private Camera mainCam;                 // レイ判定用カメラ
    private Transform holdPosition;         // 物体の移動先となる座標
    private Rigidbody grabbedRb;            // 掴んでいるオブジェクトの物理挙動
    private Collider grabbedCollider;       // 掴んでいるオブジェクトのコライダー
    private Collider playerCollider;        // プレイヤー自身のコライダー

    // 掴む前の物理パラメータを保存
    private bool originalUseGravity;
    private float originalDamping;


    void Awake()
    {
        // プレイヤー自身のコライダーを取得
        playerCollider = GetComponent<Collider>();

        // インスペクターでカメラが未設定の場合のフォールバック
        if (cameraTransform == null)
        {
            if (Camera.main != null)
            {
                cameraTransform = Camera.main.transform;
            }
            else
            {
                Debug.LogError("カメラが設定されていません！インスペクターの 'Camera Transform' にカメラをアサインしてください。");
            }
        }

        // 指定されたカメラから Camera コンポーネントを取得
        if (cameraTransform != null)
        {
            mainCam = cameraTransform.GetComponent<Camera>();
            if (mainCam == null)
            {
                mainCam = Camera.main;
            }
        }

        // 独立した HoldPosition オブジェクトを自動作成
        GameObject holdObj = new GameObject("HoldPosition");
        holdPosition = holdObj.transform;
    }

    void Update()
    {
        // プレイヤーの頭上基準に配置
        if (holdPosition != null)
        {
            holdPosition.position = transform.TransformPoint(holdLocalPosition);
        }

        // デバッグ用：レイの視覚化
        if (cameraTransform != null && mainCam != null)
        {
            Ray ray = mainCam.ViewportPointToRay(new Vector3(0.5f, 0.5f, 0f));

            if (Physics.Raycast(ray, out RaycastHit hit, maxGrabDistance))
            {
                Debug.DrawRay(ray.origin, ray.direction * hit.distance, Color.green);
            }
            else
            {
                Debug.DrawRay(ray.origin, ray.direction * maxGrabDistance, Color.red);
            }
        }
    }

    // Input Systemからの入力イベント（Cキーなど）
    public void OnGrab(InputValue value)
    {
        if (!value.isPressed) return; // 押された瞬間のみ反応

        // 掴んでいない状態なら掴み、掴んでいる状態なら反発させて放す
        if (grabbedRb == null)
        {
            TryGrab();
        }
        else
        {
            DropAndRepel(); // ← 反発して放す処理に変更
        }
    }

    void FixedUpdate()
    {
        // 掴んでいる間、プレイヤーの頭上の位置へ物理的に滑らかに引き寄せる
        if (grabbedRb != null)
        {
            Vector3 targetPos = holdPosition.position;
            Vector3 direction = targetPos - grabbedRb.position;

            float moveSpeed = 15f;

            grabbedRb.linearVelocity = direction * moveSpeed;
            grabbedRb.angularVelocity = Vector3.zero;
        }
    }

    // 画面中央の照準からレイを飛ばして掴む処理
    void TryGrab()
    {
        if (mainCam == null || cameraTransform == null) return;

        Ray ray = mainCam.ViewportPointToRay(new Vector3(0.5f, 0.5f, 0f));

        if (Physics.Raycast(ray, out RaycastHit hit, maxGrabDistance))
        {
            if (hit.collider.CompareTag(grabbableTag))
            {
                if (hit.collider.TryGetComponent(out Rigidbody rb))
                {
                    grabbedRb = rb;
                    grabbedCollider = hit.collider;

                    // プレイヤーと掴んだオブジェクトの衝突を一時的に無効化
                    if (playerCollider != null && grabbedCollider != null)
                    {
                        Physics.IgnoreCollision(playerCollider, grabbedCollider, true);
                    }

                    // 掴む前の物理パラメータを保存
                    originalUseGravity = grabbedRb.useGravity;
                    originalDamping = grabbedRb.linearDamping;

                    // 掴んでいる間は重力をオフ、減衰を上げる
                    grabbedRb.useGravity = false;
                    grabbedRb.linearDamping = 10f;
                }
            }
        }
    }

    // 物体を反発させて放す処理
    void DropAndRepel()
    {
        if (grabbedRb == null) return;

        // プレイヤーとの衝突判定を元に戻す
        if (playerCollider != null && grabbedCollider != null)
        {
            Physics.IgnoreCollision(playerCollider, grabbedCollider, false);
        }

        // 物理パラメータを元の状態に戻す
        grabbedRb.useGravity = originalUseGravity;
        grabbedRb.linearDamping = originalDamping;

        // --- 反発（弾き飛ばし）の力を加える ---
        // カメラの向いている方向（または前方向）に勢いよく力を加える
        Vector3 repelDir = cameraTransform != null ? cameraTransform.forward : transform.forward;
        grabbedRb.AddForce(repelDir * repelForce, ForceMode.Impulse);

        grabbedRb = null;
        grabbedCollider = null;
    }

    // --- 【デバッグ用】HoldPosition の位置に赤い球を表示する ---
    void OnDrawGizmos()
    {
        if (holdPosition != null)
        {
            Gizmos.color = Color.red;
            Gizmos.DrawWireSphere(holdPosition.position, 0.2f);
        }
    }
}