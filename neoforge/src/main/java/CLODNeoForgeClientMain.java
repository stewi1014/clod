import link.lenqua.clod.CLODClientMain;
import net.neoforged.fml.common.Mod;
import net.neoforged.bus.api.IEventBus;
import net.neoforged.fml.ModContainer;
import net.neoforged.fml.event.lifecycle.FMLCommonSetupEvent;

@Mod("clod")
public class CLODNeoForgeClientMain extends CLODClientMain {
	public CLODNeoForgeClientMain(IEventBus modEventBus, ModContainer modContainer) {
		modEventBus.addListener(this::commonSetup);
	}

	private void commonSetup(FMLCommonSetupEvent event) {

	}
}
