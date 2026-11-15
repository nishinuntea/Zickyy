/*==============================================================================

    ジャンプ台（プレイヤー）		 [PlayerLauncher.cs]
														 Author : Nishimura
														 Date   : 2026/11/15
--------------------------------------------------------------------------------

==============================================================================*/
using UnityEngine;

[RequireComponent(typeof(CharacterController))]
public class PlayerLauncher : MonoBehaviour
{
    public float gravity = 20f;   // 跳ね上げ中の重力（正の値）

    private CharacterController cc;
    private float verticalSpeed;
    private bool isLaunching = false;

    void Awake()
    {
        cc = GetComponent<CharacterController>();
    }

    public void Launch(float height)
    {
        Debug.Log("Launch呼ばれた");
        // 指定の高さに届く初速度
        verticalSpeed = Mathf.Sqrt(2f * gravity * height);
        isLaunching = true;
    }

    void Update()
    {
        if (!isLaunching) return;

        verticalSpeed -= gravity * Time.deltaTime;
        cc.Move(Vector3.up * verticalSpeed * Time.deltaTime);

        // 頂点に達した、または天井にぶつかったら終了
        if (verticalSpeed <= 0f || (cc.collisionFlags & CollisionFlags.Above) != 0)
        {
            isLaunching = false;
        }
    }
}