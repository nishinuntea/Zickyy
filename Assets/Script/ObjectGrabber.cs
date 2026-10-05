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
    [Header("保持位置（プレイヤーからの距離）")]
    public Vector3 holdLocalPosition = new Vector3(0.5f, 0f, 2f);

    [Header("掴めるオブジェクトとの距離")]
    [Range(1f, 50f)]
    public float maxGrabDistance = 10f;

    [Header("対象オブジェクトのレイヤー")]
    public LayerMask grabbableLayer;

    // 内部処理用の変数
    private Camera mainCam;                     // 画面中央からのレイ判定用カメラ
    private Transform holdPosition;             // 物体の移動先となる座標
    private Rigidbody grabbedRb;                // 掴んでいるオブジェクトの物理挙動
    private float followSpeed = 15f;            // 手元への追従速度
    private readonly float pullSpeed = 25f;     // 引き寄せるスピード

    void Awake()
    {
        // メインカメラの取得
        mainCam = Camera.main;

        // プレイヤーの子として「HoldPosition」を自動作成して位置を反映
        GameObject holdObj = new GameObject("HoldPosition");
        holdPosition = holdObj.transform;
        holdPosition.SetParent(transform);
        holdPosition.localPosition = holdLocalPosition;
    }

    // New Input System：Cキー入力イベント
    public void OnGrab(InputValue value)
    {
        if (value.isPressed)
        {
            TryGrab(); // 押した瞬間：掴む
        }
        else
        {
            Drop();    // 離した瞬間：放す
        }
    }

    void FixedUpdate()
    {
        // 掴んでいる間、毎フレーム手元の位置へ滑らかに引き寄せ続ける
        if (grabbedRb != null)
        {
            Vector3 targetPos = holdPosition.position;

            // 手元に吸い寄せる物理演算
            Vector3 forceDirection = targetPos - grabbedRb.position;
            grabbedRb.linearVelocity = forceDirection * pullSpeed;
        }
    }

    // 画面中央の照準からレイを飛ばして掴む処理
    void TryGrab()
    {
        Ray ray = mainCam.ViewportPointToRay(new Vector3(0.5f, 0.5f, 0f));

        if (Physics.Raycast(ray, out RaycastHit hit, maxGrabDistance, grabbableLayer))
        {
            if (hit.collider.TryGetComponent(out Rigidbody rb))
            {
                grabbedRb = rb;

                // 掴んでいる間は重力をオフ、減衰を上げて手元でピタッと止まるようにする
                grabbedRb.useGravity = false;
                grabbedRb.linearDamping = 15f;
            }
        }
    }

    // 物体を放す処理
    void Drop()
    {
        if (grabbedRb == null) return;

        // 物理パラメータを元の状態に戻す
        grabbedRb.useGravity = true;
        grabbedRb.linearDamping = 0f;

        grabbedRb = null;
    }
}
