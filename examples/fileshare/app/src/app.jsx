import { IoCogSharp } from 'solid-icons/io';
import { IoAdd } from 'solid-icons/io';

import Files from './files';

import './app.css';

function App() {
	return (
		<>
			<div class="toolbar">
				<button><IoCogSharp /></button>
				<button><IoAdd /></button>
			</div>

			<Files />
		</>
	);
}

export default App;
